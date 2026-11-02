/*
 * debug_test -- the debugger contract (core/include/arcdebug.h) against a
 * running machine: attach stops it (before the session starts, too), the CPU
 * view and its edits, step / step over / step out / run to / scanline /
 * frame, disassembly with labels and the PC line, breakpoints (execute, read,
 * write) with conditions, memory in the three spaces, the UVI snapshot /
 * character screen / views / objects / hit test, I/O, the cartridge, symbols
 * in every format, the trace ring, Save, the prompt and its completion, the
 * FujiNet trace (wired through the cartridge's real transport to a stand-in
 * FujiNet on loopback), detach, and a power cycle with the debugger attached.
 *
 * The machine is the host's (core/arc/host.h) running the baked CONFIG client
 * (there is no BIOS and no ROM to import), then a 2K game assembled in this
 * file; it does not depend on pixels, only on structure.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "dasm2650.h"
#include "arcmap.h"
#include "debugger_internal.h"
#include "fuji_mailbox.h"
#include "fujitrace.h"
#include "host.h"
#include "test_tmpdir.h"

static int failures;
static int last_ok = 1;
static void check_(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
    last_ok = ok;
}
#define check(ok, what) check_((ok) != 0, what)
/* what the last failed check saw */
#define show(fmt, ...) do { if (!last_ok) printf("      " fmt "\n", __VA_ARGS__); } while (0)

static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static int wait_stopped(arcdebug *d, int want, int timeout_ms)
{
    int waited = 0;
    while (waited < timeout_ms) {
        if (!!arcdebug_is_stopped(d) == want) return 1;
        sleep_ms(2); waited += 2;
    }
    return 0;
}

/* Resume and wait for the next stop. */
static int run_until_stop(arcdebug *d, int timeout_ms)
{
    unsigned g = arcdebug_generation(d);
    int waited = 0;
    arcdebug_resume(d);
    while (waited < timeout_ms) {
        if (arcdebug_is_stopped(d) && arcdebug_generation(d) != g) return 1;
        sleep_ms(1); waited += 1;
    }
    return 0;
}

static int stop_now(arcdebug *d)
{
    arcdebug_stop(d);
    return wait_stopped(d, 1, 3000);
}

static arcdebug_cpu cpu(arcdebug *d)
{
    arcdebug_cpu c;
    arcdebug_cpu_get(d, &c);
    return c;
}

static char reason[128];
static int why(arcdebug *d)
{
    int addr = -1;
    arcdebug_stop_reason(d, reason, sizeof reason, &addr);
    return addr;
}

static long file_size(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 ? (long)st.st_size : -1;
}

static void write_text(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (f) { fputs(text, f); fclose(f); }
}

static int find_line(const arcdebug_line *l, int n, const char *prefix)
{
    for (int i = 0; i < n; i++)
        if (strncmp(l[i].disasm, prefix, strlen(prefix)) == 0) return i;
    return -1;
}

/* ---- a stand-in FujiNet on loopback (POSIX) ----------------------------------- */

#ifndef _WIN32
typedef struct { int listen_fd, fd, frames, port; atomic_int stop; } fake_fujinet;

/* A SLIP-framed FujiBus ACK from the FUJI device with one payload byte. */
static size_t ack_frame(uint8_t *out)
{
    uint8_t pkt[7] = { 0x70, 0x06, 7, 0, 0, 0, 0 };
    uint16_t ck = 0;
    size_t o = 0;
    for (int i = 0; i < 7; i++) {
        ck = (uint16_t)(ck + pkt[i]);
        ck = (uint16_t)((ck >> 8) + (ck & 0xFF));
    }
    pkt[4] = (uint8_t)ck;
    out[o++] = 0xC0;
    for (int i = 0; i < 7; i++) out[o++] = pkt[i];
    out[o++] = 0xC0;
    return o;
}

/* Answers every request frame with an ACK until the console hangs up, then
 * waits for the next console (the test power-cycles the machine) until told to
 * stop. */
static void *fake_main(void *arg)
{
    fake_fujinet *f = arg;
    uint8_t c, reply[16];

    for (;;) {
        int ends = 0;

        f->fd = accept(f->listen_fd, NULL, NULL);
        if (f->fd < 0)
            break;
        if (atomic_load(&f->stop)) {
            close(f->fd);
            break;
        }
        while (recv(f->fd, &c, 1, 0) == 1) {
            if (c != 0xC0) continue;
            if (++ends == 2) {          /* a frame: leading and trailing delimiter */
                send(f->fd, reply, ack_frame(reply), 0);
                f->frames++;
                ends = 0;
            }
        }
        close(f->fd);
    }
    return NULL;
}
#endif

/* ---- a game cartridge, to run known code ---------------------------------------- */

/* 2K at $0000: a little program with one of everything the breakpoints watch.
 *   0000 lodi,r0 $77
 *   0002 stra,r0 $18F0     write the UVI (object 0's y register)
 *   0005 loda,r0 $18F0     read it back
 *   0008 bsta,un $0020     call: $0020 calls $0028, which returns, and it returns
 *   000B lodi,r1 $03
 *   000D bdrr,r1 $000D     three trips
 *   000F loda,r0 $0060     reads the table byte ($5A)
 *   0012 bctr,un $0012     idles
 *   0040 bsta,un $0030 / 0043 lodi,r0 $00 / 0045 bctr,un $0045
 *   0030 lodi,r0 $01 / 0032 retc,eq (not taken) / 0033 retc,un
 */
static void build_game(uint8_t *img)
{
    static const uint8_t main_code[] = {
        0x04, 0x77, 0xCC, 0x18, 0xF0, 0x0C, 0x18, 0xF0, 0x3F, 0x00, 0x20, 0x05, 0x03,
        0xF9, 0x7E, 0x0C, 0x00, 0x60, 0x1B, 0x7E,
    };
    static const uint8_t sub1[] = { 0x3F, 0x00, 0x28, 0x17 };         /* 20: bsta $28 / retc,un */
    static const uint8_t sub1b[] = { 0x06, 0x22, 0x17 };              /* 28: lodi,r2 $22 / retc,un */
    static const uint8_t sub2[] = { 0x04, 0x01, 0x14, 0x17 };         /* 30: lodi,r0 1 / retc,eq / retc,un */
    static const uint8_t caller[] = { 0x3F, 0x00, 0x30, 0x04, 0x00, 0x1B, 0x7E };

    memset(img, 0, 2048);
    memcpy(img, main_code, sizeof main_code);
    memcpy(img + 0x20, sub1, sizeof sub1);
    memcpy(img + 0x28, sub1b, sizeof sub1b);
    memcpy(img + 0x30, sub2, sizeof sub2);
    memcpy(img + 0x40, caller, sizeof caller);
    img[0x60] = 0x5A;
}

static void game_tests(arcdebug *d, const char *tmp)
{
    char out[8192], path[800], msg[256];
    arcdebug_cpu c, c2;
    arcdebug_breakpoint bps[16];
    arcdebug_line lines[32];
    arcdebug_cart cart;
    int n, i;

    /* the cartridge */
    arcdebug_cart_get(d, &cart);
    check(cart.present && cart.type == ARC_CART_STD && strcmp(cart.type_name, "Standard") == 0 && cart.rom_size == 2048,
          "cartridge: a standard 2K game");
    check(!cart.mailbox && !cart.link_up, "with no FujiNet behind it");
    arcdebug_cart_info(d, out, sizeof out);
    check(strcmp(out, "Standard cartridge, 2048 bytes") == 0, "its one line");
    arcdebug_command(d, "cart", out, sizeof out);
    check(strstr(out, "Standard") && strstr(out, "2048"), "the prompt's cart for a mapper");
    check(arcdebug_fujinet_state(d, out, sizeof out) > 0 && strstr(out, "not the FujiNet"), "and no FujiNet state");
    snprintf(path, sizeof path, "%s/game.mailbox", tmp);
    arcdebug_save(d, "mailbox", path, msg, sizeof msg);
    check(file_size(path) == 0x500, "a mapper cartridge's $2B00 page saves from what the console reads");
    check(arcdebug_space_size(d, ARC_SPACE_CART) == 2048 && arcdebug_space_size(d, ARC_SPACE_PROG) == 0x8000 &&
          arcdebug_space_size(d, ARC_SPACE_UVI) == 0x300, "the sizes of the three spaces");

    /* the program, disassembled */
    n = arcdebug_disassemble(d, 0, lines, 8, NULL);
    check(n == 8 && strncmp(lines[0].disasm, "lodi,0", 6) == 0 && strstr(lines[0].disasm, "$77") &&
          lines[0].length == 2 && strcmp(lines[0].bytes, "04 77") == 0 &&
          strncmp(lines[1].disasm, "stra,0", 6) == 0 && lines[1].length == 3 && strcmp(lines[1].bytes, "CC 18 F0") == 0,
          "the game's first instructions, with their bytes");
    show("'%s' '%s'", lines[0].disasm, lines[1].disasm);
    i = find_line(lines, n, "bsta");
    check(i >= 0 && lines[i].address == 0x008 && strstr(lines[i].disasm, "$0020"), "a call shows its 15-bit target");
    i = find_line(lines, n, "bdrr");
    check(i >= 0 && lines[i].address == 0x00D && strstr(lines[i].disasm, "$000d"), "BDRR shows its target");

    /* breakpoints on the accesses, in program order */
    {
        static const struct { int type; int lo, hi; const char *cond, *reason; int pc; } bp[] = {
            { ARCDEBUG_BP_WRITE, 0x18F0, 0x18F0, "value == $77", "write prog $18F0 = $77", 0x005 },
            { ARCDEBUG_BP_READ, 0x18F0, 0x18FF, "", "read prog $18F0 = $77", 0x008 },
            { ARCDEBUG_BP_READ, 0x0060, 0x0060, "value == $5A && addr == $60", "read prog $0060 = $5A", 0x012 },
        };
        for (i = 0; i < 3; i++)
            check(arcdebug_breakpoint_add(d, bp[i].type, ARC_SPACE_PROG, (uint16_t)bp[i].lo, (uint16_t)bp[i].hi, bp[i].cond) > 0,
                  "an access breakpoint is added");
        for (i = 0; i < 3; i++) {
            char what[96];
            snprintf(what, sizeof what, "stops at $%04X: %s", bp[i].pc, bp[i].reason);
            check(run_until_stop(d, 3000) && cpu(d).pc == bp[i].pc && strcmp((why(d), reason), bp[i].reason) == 0, what);
            show("pc %04X reason '%s'", cpu(d).pc, reason);
        }
        n = arcdebug_breakpoint_list(d, bps, 16);
        check(n == 3 && bps[0].hits == 1 && bps[1].hits == 1 && bps[2].hits == 1, "each hit exactly once");
        c = cpu(d);
        check(c.r0 == 0x5A && c.r[0][2] == 0x22 && c.r[1][1] == 0 && c.sp == 0, "R0 and R2 are what the program made them");
        arcdebug_breakpoint_clear(d);
    }

    /* an exec breakpoint at the call, stepped over: the routine, a nested call, both returns */
    arcdebug_cpu_set(d, ARC_REG_PC, 0x008);
    c = cpu(d);
    check(c.pc == 0x008 && c.sp == 0, "back at the CALL");
    arcdebug_step_over(d);
    check(wait_stopped(d, 1, 3000), "step over stops");
    c2 = cpu(d);
    check(c2.pc == 0x00B && c2.sp == 0 && c2.instructions == c.instructions + 5 && c2.r[0][2] == 0x22,
          "step over runs the call (bsta, lodi, retc, retc) and stops after it");
    check(strcmp((why(d), reason), "step over") == 0, "reason 'step over'");

    /* a loop is not stepped over: BDRR is a plain branch */
    arcdebug_run_to(d, 0x00D);
    check(wait_stopped(d, 1, 3000) && cpu(d).pc == 0x00D && cpu(d).r[0][1] == 3, "run to the BDRR");
    c = cpu(d);
    arcdebug_step_over(d);
    check(wait_stopped(d, 1, 3000) && cpu(d).instructions == c.instructions + 1 && cpu(d).pc == 0x00D &&
          cpu(d).r[0][1] == 2, "step over a branch that is not a call is one step");

    /* step into a CALL: the return stack shows the way back; then out of it, and out of a nested one */
    arcdebug_cpu_set(d, ARC_REG_PC, 0x008);
    arcdebug_step(d);
    check(wait_stopped(d, 1, 3000), "step into stops");
    c = cpu(d);
    check(c.pc == 0x020 && c.sp == 1 && c.ras[1] == 0x00B, "a step into a CALL is in the routine, one deeper, the return address pushed");
    arcdebug_step_out(d);
    check(wait_stopped(d, 1, 3000), "step out stops");
    c2 = cpu(d);
    check(c2.pc == 0x00B && c2.sp == 0 && strcmp((why(d), reason), "step out") == 0 &&
          c2.instructions == c.instructions + 4, "step out runs to the routine's RETC (the nested call and its RETC included)");
    arcdebug_cpu_set(d, ARC_REG_PC, 0x020);
    arcdebug_step(d);
    check(wait_stopped(d, 1, 3000) && cpu(d).pc == 0x028 && cpu(d).sp == 1, "into the nested routine");
    arcdebug_step_out(d);
    check(wait_stopped(d, 1, 3000) && cpu(d).pc == 0x023 && cpu(d).sp == 0, "out of it, one level");
    /* a conditional return that is not taken does not end the step out */
    arcdebug_cpu_set(d, ARC_REG_PC, 0x040);
    arcdebug_step(d);
    check(wait_stopped(d, 1, 3000) && cpu(d).pc == 0x030 && cpu(d).sp == 1, "into the routine with the conditional return");
    c = cpu(d);
    arcdebug_step_out(d);
    check(wait_stopped(d, 1, 3000) && cpu(d).pc == 0x043 && cpu(d).sp == 0 && cpu(d).instructions == c.instructions + 3,
          "step out passes a RETC that is not taken and stops after the one that is");

    /* the registers a program leaves: R1-R3 in both banks, the status bytes */
    arcdebug_cpu_set(d, ARC_REG_PC, 0x00B);
    arcdebug_step(d);
    check(wait_stopped(d, 1, 3000), "one step");
    c = cpu(d);
    check(c.r[0][1] == 3 && c.rs == 0 && c.cc == 1 && c.pc == 0x00D, "LODI,R1 $03 left R1 = 3 and the condition code positive");
    arcdebug_cpu_set(d, ARC_REG_RS, 1);
    c2 = cpu(d);
    check(c2.rs == 1 && c2.r[1][1] != 3 && c2.r[0][1] == 3, "RS swaps the register set in use; the other set kept R1");
    arcdebug_cpu_set(d, ARC_REG_R1B, 0x44);
    check(cpu(d).r[1][1] == 0x44, "R1' takes an edit (RS = 1: the one in use)");
    arcdebug_cpu_set(d, ARC_REG_RS, 0);
    check(cpu(d).r[1][1] == 0x44 && cpu(d).r[0][1] == 3, "and back");

    /* idle, PAL: the beam, scanlines, frames */
    arcdebug_cpu_set(d, ARC_REG_PC, 0x012);
    c = cpu(d);
    check(c.machine == ARC_MACHINE_ARCADIA_PAL, "the PAL machine");
    check(c.vpos >= 0 && c.vpos < 312 && c.hpos >= 0 && c.hpos < ARCSESSION_FB_WIDTH, "the beam is in the PAL raster");
    arcdebug_scanline(d, 1);
    check(wait_stopped(d, 1, 3000) && cpu(d).vpos == (c.vpos + 1) % 312, "scanline+1 on a 312-line frame");
    c = cpu(d);
    arcdebug_scanline(d, 312);
    check(wait_stopped(d, 1, 3000) && cpu(d).vpos == c.vpos && cpu(d).frame == c.frame + 1, "312 scanlines is one frame");
    c = cpu(d);
    arcdebug_frame(d, 2);
    check(wait_stopped(d, 1, 3000) && cpu(d).frame == c.frame + 2, "frame+2");
    {
        arcdebug_uvi u;
        arcdebug_uvi_get(d, &u);
        check(u.lines == 312, "the UVI knows it has 312 lines");
    }
    (void)c2;
}

/* ---- the program under test --------------------------------------------------- */

int main(void)
{
    static arcdebug_fuji_packet pk[FUJITRACE_MAX];
    char tmp[512], out[16384], path[800], msg[300], boip[64] = "";
    int call_pc = -1;
    static uint32_t img[ARCDEBUG_UVI_VIEW_MAX_PIXELS * 4], img2[ARCDEBUG_UVI_VIEW_MAX_PIXELS * 4];
    arc_host_opts_t ho;
    arcdebug *d;
    arcdebug_cpu c, c2, saved;
    arcdebug_line lines[64];
    arcdebug_breakpoint bps[16];
    int n, w, h, pc_line = -1, id, id2, i, fresh;
#ifndef _WIN32
    fake_fujinet fake;
    pthread_t fake_th;
    int fake_up = 0;
#endif

    test_tmpdir(tmp, sizeof tmp, "dbg");

    d = arcdebug_create();
    check(d != NULL, "an engine");
    if (!d) return 1;

    /* ---- with no machine at all ---- */
    check(!arcdebug_is_attached(d) && !arcdebug_is_stopped(d), "a new engine is detached");
    c = cpu(d);
    check(c.pc == 0 && c.cycles == 0, "no machine: a zero CPU view");
    check(arcdebug_read(d, ARC_SPACE_PROG, 0, (uint8_t *)out, 4) == 0, "no machine: no memory");
    check(!arcdebug_uvi_view(d, ARCDEBUG_UVI_FRAME, ARCDEBUG_UVI_L_ALL, 0, 1, NULL, 0, &w, &h), "no machine: no UVI view");
    check(arcdebug_uvi_cells(d, (arcdebug_uvi_cell *)out) == 0, "no machine: no character screen");
    check(arcdebug_cart_info(d, out, sizeof out) > 0, "no machine: a cartridge line");
    arcdebug_command(d, "regs", out, sizeof out);
    check(out[0] != '\0', "no machine: the prompt still answers");
    check(arcdebug_label_address(d, "UVI_OBJ0_Y") == 0x18F0 && arcdebug_label_address(d, "reset") == 0,
          "built-in labels need no machine");

    /* ---- the FujiNet trace is installed first, from session start ---- */
    arcdebug_fujinet_install(d);
#ifndef _WIN32
    {
        struct sockaddr_in a;
        socklen_t al = sizeof a;
        int one = 1;

        memset(&fake, 0, sizeof fake);
        fake.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        setsockopt(fake.listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        if (fake.listen_fd >= 0 && bind(fake.listen_fd, (struct sockaddr *)&a, sizeof a) == 0 &&
            listen(fake.listen_fd, 1) == 0 &&
            getsockname(fake.listen_fd, (struct sockaddr *)&a, &al) == 0 &&
            pthread_create(&fake_th, NULL, fake_main, &fake) == 0) {
            fake.port = ntohs(a.sin_port);
            snprintf(boip, sizeof boip, "127.0.0.1:%d", fake.port);
            fake_up = 1;
        }
    }
#endif

    /* ---- attached before the machine exists: it stops on its first instruction ---- */
    arcdebug_attach(d);
    check(arcdebug_is_attached(d), "attached with no machine to hook yet");
    id = arcdebug_breakpoint_add(d, ARCDEBUG_BP_WRITE, ARC_SPACE_PROG, 0x19F9, 0x19F9, "addr == $19F9");
    id2 = arcdebug_breakpoint_add(d, ARCDEBUG_BP_READ, ARC_SPACE_PROG, 0x2C09, 0x2C09, "value == $46");
    check(id > 0 && id2 > 0, "a UVI write and a mailbox read breakpoint, set before the machine");

    memset(&ho, 0, sizeof ho);
    ho.machine = ARC_MACHINE_ARCADIA;
    ho.boip_hostport = boip[0] ? boip : NULL;
    /* The host's thread starts running as soon as arc_host_start returns, and the
     * hook goes in a moment later: on a slow or loaded machine the console may
     * have run some instructions first. Power cycle until the hook is first. */
    for (i = 0; i < 8; i++) {
        if (arc_host_start(&ho) != 0) {
            printf("FAIL: the host does not start: %s\n", arc_host_last_error());
            return 1;
        }
        arcdebug_power_up(d);
        if (!wait_stopped(d, 1, 3000))
            break;
        c = cpu(d);
        if (c.pc == 0 && c.instructions == 0)
            break;
        arcdebug_power_down(d);
        arc_host_stop();
    }
    check(wait_stopped(d, 1, 3000), "the new machine stops in the debugger at once");
    c = cpu(d);
    /* Eight tries and still late: say so, and carry on from the reset vector
     * (what is skipped is the boot-time part). */
    fresh = c.pc == 0 && c.instructions == 0;
    if (!fresh) {
        printf("note: the machine ran before the hook went in (pc %04X, %llu instructions): "
               "the boot-time checks are skipped\n", c.pc, (unsigned long long)c.instructions);
        arcdebug_cpu_set(d, ARC_REG_PC, 0);
        c = cpu(d);
    }
    check(c.pc == 0 && c.machine == ARC_MACHINE_ARCADIA, "on the reset vector");
    if (fresh) {
        check(c.instructions == 0 && c.cycles == 0, "before its first instruction");
        check(c.vpos == 0 && c.frame == 0 && c.sp == 0 && c.rs == 0, "the beam and the stack start at zero");
        check(c.psl == (S2650_COM | S2650_WC) && c.com && c.wc && c.psu == 0, "reset leaves PSL = COM|WC and PSU = 0");
    }
    check(why(d) == 0 && strcmp(reason, "stopped") == 0, "the stop reason is 'stopped', at the PC");
    n = arcdebug_disassemble(d, 0, lines, 3, &pc_line);
    check(n == 3 && pc_line == 0 && lines[0].is_pc, "disassembly marks the PC's line");
    check(lines[0].length >= 1 && lines[0].length <= 3 && strcmp(lines[0].label, "reset") == 0,
          "CONFIG's first instruction is labelled 'reset'");
    check(lines[1].address == lines[0].length && lines[1].label[0] == '\0', "the next line follows the instruction");
    arcdebug_step(d);
    check(wait_stopped(d, 1, 2000), "stopped after a step");
    c2 = cpu(d);
    check(c2.instructions == c.instructions + 1 && c2.cycles > c.cycles && c2.cycles <= c.cycles + 12,
          "a step runs one instruction (6, 9 or 12 cycles)");
    check(why(d) == c2.pc && strcmp(reason, "step") == 0, "the reason is 'step', at the new PC");

    /* the breakpoints set before the machine existed fire during CONFIG's boot */
    if (fresh) {
        int saw_uvi = 0, saw_mbx = 0;
        for (i = 0; i < 8 && !(saw_uvi && saw_mbx); i++) {
            if (!run_until_stop(d, 5000)) break;
            why(d);
            if (strstr(reason, "write prog $19F9 = $")) saw_uvi = 1;
            if (strstr(reason, "read prog $2C09 = $46")) saw_mbx = 1;
        }
        check(saw_uvi, "a UVI-write breakpoint with an `addr` condition");
        check(saw_mbx, "a mailbox-read breakpoint with a `value` condition (the cartridge answers 'F')");
        n = arcdebug_breakpoint_list(d, bps, 16);
        check(n == 2 && bps[0].hits >= 1 && bps[1].hits >= 1 && bps[0].type == ARCDEBUG_BP_WRITE &&
              bps[0].space == ARC_SPACE_PROG && bps[1].start == 0x2C09,
              "listed with their hit counts, space and range");
    }
    arcdebug_breakpoint_clear(d);
    check(arcdebug_breakpoint_list(d, bps, 16) == 0, "cleared");

    /* let CONFIG settle, then stop wherever it is */
    arcdebug_resume(d);
    {
        int waited = 0;
        while (arc_host_frame_count() < 90 && waited < 5000) { sleep_ms(10); waited += 10; }
        check(arc_host_frame_count() >= 90, "the machine runs with the debugger attached");
    }
    check(stop_now(d), "stop");
    check(why(d) >= 0 && strcmp(reason, "stopped") == 0, "stopped on request");

    /* ---- the CPU view ---- */
    c = cpu(d);
    check(c.pc >= 0 && c.pc < 0x8000 && c.pc == (c.page | c.iar) && c.iar < 0x2000 && (c.page & ~0x6000) == 0,
          "PC is 15 bits: page + iar");
    check((c.psu & 0x67) == (c.flag << 6 | c.ii << 5 | c.sp) && c.sp < 8, "PSU and its decode agree");
    check((c.psl & 0xc0) >> 6 == c.cc && c.cc <= 2 && ((c.psl >> 5) & 1) == c.idc && ((c.psl >> 4) & 1) == c.rs &&
          ((c.psl >> 3) & 1) == c.wc && ((c.psl >> 2) & 1) == c.ovf && ((c.psl >> 1) & 1) == c.com && (c.psl & 1) == c.c,
          "PSL and its decode agree");
    check(c.frame >= 80 && c.cycles > 100000 && c.instructions > 10000, "time has passed");
    check(c.vpos >= 0 && c.vpos < 262 && c.hpos >= 0 && c.hpos < ARCSESSION_FB_WIDTH, "the beam is in the NTSC raster");
    check(c.r[0][0] == c.r0 && c.r[1][0] == c.r0, "R0 is the same in both banks");
    check(!c.halted && c.illegal_count == 0, "CONFIG is not halted and ran no illegal opcode");

    /* ---- register edits (restored afterwards) ---- */
    saved = c;
    arcdebug_cpu_set(d, ARC_REG_R0, 0x5A);
    arcdebug_cpu_set(d, ARC_REG_R3, 0xC3);
    arcdebug_cpu_set(d, ARC_REG_R2B, 0x66);
    arcdebug_cpu_set(d, ARC_REG_C, !saved.c);
    arcdebug_cpu_set(d, ARC_REG_II, !saved.ii);
    arcdebug_cpu_set(d, ARC_REG_PC, 0x321);
    c2 = cpu(d);
    check(c2.r0 == 0x5A && c2.r[c.rs][3] == 0xC3 && c2.r[1][2] == 0x66, "R0 and registers (the set in use, and set 1) take edits");
    check(c2.c == !saved.c && c2.ii == !saved.ii, "C and II too");
    check(c2.pc == 0x321 && c2.iar == 0x321 && c2.page == 0, "PC takes an edit");
    arcdebug_cpu_set(d, ARC_REG_PC, 0x4123);
    c2 = cpu(d);
    check(c2.pc == 0x4123 && c2.page == 0x4000 && c2.iar == 0x0123, "and its page");
    arcdebug_cpu_set(d, ARC_REG_IAR, 0x0456);
    check(cpu(d).pc == 0x4456, "IAR leaves the page alone");
    arcdebug_cpu_set(d, ARC_REG_PSU, 0x05);
    check(cpu(d).sp == 5 && cpu(d).psu == 0x05, "PSU takes a byte (SP = 5)");
    arcdebug_cpu_set(d, ARC_REG_SP, 2);
    check(cpu(d).sp == 2, "SP");
    arcdebug_cpu_set(d, ARC_REG_RAS0 + 4, 0x1234);
    check(cpu(d).ras[4] == 0x1234, "a return stack entry takes an edit");
    arcdebug_cpu_set(d, ARC_REG_CC, 2);
    check(cpu(d).cc == 2 && (cpu(d).psl & 0xc0) == 0x80, "CC = negative");
    arcdebug_cpu_set(d, ARC_REG_PSL, 0x1a);
    c2 = cpu(d);
    check(c2.psl == 0x1a && c2.rs == 1 && c2.wc == 1 && c2.com == 1 && c2.cc == 0, "PSL takes a whole byte");
    /* restore: PSL first (RS swaps the sets), then the registers it swapped */
    arcdebug_cpu_set(d, ARC_REG_PSL, saved.psl);
    arcdebug_cpu_set(d, ARC_REG_PSU, saved.psu);
    arcdebug_cpu_set(d, ARC_REG_R0, saved.r0);
    for (i = 1; i < 4; i++) {
        arcdebug_cpu_set(d, ARC_REG_R1 + i - 1, saved.r[saved.rs][i]);
        arcdebug_cpu_set(d, ARC_REG_R1B + i - 1, saved.r[1][i]);
    }
    arcdebug_cpu_set(d, ARC_REG_RAS0 + 4, saved.ras[4]);
    arcdebug_cpu_set(d, ARC_REG_PC, saved.pc);
    c2 = cpu(d);
    check(c2.pc == saved.pc && c2.psl == saved.psl && c2.psu == saved.psu && c2.r0 == saved.r0 &&
          memcmp(c2.r, saved.r, sizeof c2.r) == 0, "the CPU is as it was");
    show("psl %02X/%02X r %02X %02X %02X / %02X %02X %02X", c2.psl, saved.psl, c2.r[0][1], c2.r[0][2], c2.r[0][3],
         c2.r[1][1], c2.r[1][2], c2.r[1][3]);

    /* ---- the disassembly ---- */
    n = arcdebug_disassemble(d, 0, lines, 20, NULL);
    check(n == 20 && strcmp(lines[0].label, "reset") == 0 && lines[0].bytes[0] != '\0' && lines[0].disasm[0] != '\0',
          "the reset vector is labelled");
    {
        int ok = 1, addr = 0;
        for (i = 0; i < n; i++) {
            if (lines[i].address != addr) ok = 0;
            addr = (addr + lines[i].length) & 0x7fff;
        }
        check(ok, "consecutive lines abut, by each instruction's length");
    }
    check(arcdebug_row_address(d, 0x100, 0) == 0x100, "no rows is the same address");
    {
        arcdebug_line two[2];
        arcdebug_disassemble(d, 0x100, two, 2, NULL);
        check(arcdebug_row_address(d, 0x100, 2) == two[1].address + two[1].length, "two rows after $100");
        check(arcdebug_row_address(d, (uint16_t)(two[1].address + two[1].length), -2) == 0x100, "and two rows back");
    }
    n = arcdebug_disassemble(d, (uint16_t)arcdebug_row_address(d, saved.pc, -4), lines, 16, &pc_line);
    check(pc_line >= 0 && lines[pc_line].address == saved.pc && lines[pc_line].is_pc, "the PC's line is found among the rows");
    check(arcdebug_label_address(d, "UVI_OBJ0_Y") == 0x18F0 && arcdebug_label_address(d, "nonesuch") == -1, "label lookup");
    check(arcdebug_label_address(d, "FN_R_ACKSEQ") == 0x2C00 && arcdebug_label_address(d, "FN_R_DATA") == 0x2B00 &&
          arcdebug_label_address(d, "FN_R_TICK_LO") == 0x2C0E && arcdebug_label_address(d, "FN_HOT_SWAP") == 0x2DFE &&
          arcdebug_label_address(d, "FN_HOT_BANK") == 0x2D80 && arcdebug_label_address(d, "FN_H_REGSEL") == 0x2D00,
          "the mailbox labels come from fuji_mailbox.h");
    check(arcdebug_label_address(d, "UVI_PAL1") == 0x19F9 && arcdebug_label_address(d, "UVI_UDC38") == 0x1980 &&
          arcdebug_label_address(d, "UVI_SCR2") == 0x1A00, "the UVI's registers are labelled");
    arcdebug_address_label(d, 0x19FC, out, sizeof out);
    check(strcmp(out, "UVI_COLL_CHARS") == 0, "address to label");

    /* ---- memory in the three spaces ---- */
    {
        uint8_t b[8], v, p0[4], c0[4];
        check(arcdebug_read(d, ARC_SPACE_PROG, 0, p0, 4) == 4 && arcdebug_read(d, ARC_SPACE_CART, 0, c0, 4) == 4 &&
              memcmp(p0, c0, 4) == 0, "PROG $0000 is the cartridge image's offset 0");
        check(arcdebug_read(d, ARC_SPACE_PROG, 0x2C09, b, 3) == 3 && b[0] == 'F' && b[1] == 'N' && b[2] == FN_PROTO_VER,
              "the cartridge's status page reads through the FujiNet cartridge ('FN', protocol)");
        check(arcdebug_read(d, ARC_SPACE_CART, 0x1C09, b, 2) == 2 && b[0] == 'F' && b[1] == 'N',
              "and in the CART space it is image $1C09");
        check(arcdebug_read(d, ARC_SPACE_PROG, 0x7FFE, b, 4) == 4 && b[2] == 0xFF && b[3] == 0xFF, "PROG reads $FF past $7FFF");
        check(arcdebug_space_size(d, ARC_SPACE_CART) == ARCMAP_STORE, "the FujiNet cartridge's CART space is its whole store");
        arcdebug_read(d, ARC_SPACE_UVI, 0x1B0, &b[7], 1);
        arcdebug_write(d, ARC_SPACE_UVI, 0x1B0, 0x5A);
        arcdebug_read(d, ARC_SPACE_UVI, 0x1B0, &v, 1);
        check(v == 0x5A, "the UVI takes a write (a UDC row)");
        arcdebug_read(d, ARC_SPACE_PROG, 0x19B0, &v, 1);
        check(v == 0x5A, "which is the same byte at $19B0 in PROG space");
        arcdebug_write(d, ARC_SPACE_PROG, 0x19B0, 0x3C);
        arcdebug_read(d, ARC_SPACE_UVI, 0x1B0, &v, 1);
        check(v == 0x3C, "and a PROG write into the UVI window reaches it");
        {
            arcdebug_uvi u;
            arcdebug_uvi_get(d, &u);
            check(u.chars[0x3E][0] == 0x3C, "with the chip's write effect: the glyph table sees the new UDC");
        }
        arcdebug_write(d, ARC_SPACE_UVI, 0x1B0, b[7]);
        arcdebug_read(d, ARC_SPACE_PROG, 0x0, &b[5], 1);
        arcdebug_write(d, ARC_SPACE_PROG, 0x0, (uint8_t)(b[5] ^ 0xFF));
        arcdebug_read(d, ARC_SPACE_PROG, 0x0, &v, 1);
        check(v == b[5] || v == (uint8_t)(b[5] ^ 0xFF), "a PROG write to the image does not crash");
        arcdebug_write(d, ARC_SPACE_PROG, 0x0, b[5]);
        arcdebug_write(d, ARC_SPACE_UVI, 0x400, 0x11);
        check(arcdebug_read(d, ARC_SPACE_UVI, 0x2FE, &v, 1) == 1, "a write past the UVI's window is ignored");
    }

    /* ---- step over, step out, run to, on CONFIG ---- */
    {
        static arcdebug_trace tr[4096];
        int ret = 0;

        arcdebug_trace_enable(d, 1);
        check(arcdebug_trace_enabled(d), "trace on");
        arcdebug_frame(d, 1);
        check(wait_stopped(d, 1, 3000), "stopped after a frame");
        n = arcdebug_trace_read(d, tr, 4096);
        for (i = n - 1; i >= 0 && call_pc < 0; i--)
            if (tr[i].bytes[0] == 0x3F)            /* BSTA,un */
                call_pc = tr[i].pc;
        arcdebug_trace_enable(d, 0);
        check(call_pc >= 0, "CONFIG calls a routine within a frame (found in the trace)");

        if (call_pc >= 0) {
            ret = (call_pc + 3) & 0x7fff;
            id = arcdebug_breakpoint_add(d, ARCDEBUG_BP_EXEC, ARC_SPACE_PROG, (uint16_t)call_pc, (uint16_t)call_pc, "");
            check(run_until_stop(d, 5000), "run to the call");
            arcdebug_breakpoint_remove(d, id);
            c = cpu(d);
            check(c.pc == call_pc && strstr((why(d), reason), "breakpoint: exec") != NULL, "an execute breakpoint, with its reason");
            n = arcdebug_disassemble(d, (uint16_t)call_pc, lines, 1, NULL);
            check(n == 1 && lines[0].has_breakpoint == 0 && strncmp(lines[0].disasm, "bsta", 4) == 0,
                  "the instruction is a CALL (and the breakpoint was removed)");
            arcdebug_step_over(d);
            check(wait_stopped(d, 1, 5000), "step over stops");
            c2 = cpu(d);
            check(c2.pc == ret && c2.sp == c.sp && (why(d), strcmp(reason, "step over") == 0),
                  "step over a CALL returns to the next instruction, same stack");
            check(c2.cycles > c.cycles + 12, "having run the routine");
            arcdebug_cpu_set(d, ARC_REG_PC, (uint16_t)call_pc);
            arcdebug_step(d);
            check(wait_stopped(d, 1, 2000), "step into stops");
            c2 = cpu(d);
            check(c2.sp == c.sp + 1 && c2.ras[c2.sp] == ret, "a step into a CALL is in the routine, one deeper, with the return pushed");
            arcdebug_step_out(d);
            check(wait_stopped(d, 1, 5000), "step out stops");
            c2 = cpu(d);
            check(c2.pc == ret && c2.sp == c.sp && (why(d), strcmp(reason, "step out") == 0),
                  "step out returns to the caller's next instruction");
        }

        arcdebug_trace_clear(d);
        arcdebug_trace_enable(d, 1);
        arcdebug_run_to(d, (uint16_t)call_pc);
        check(wait_stopped(d, 1, 5000) && cpu(d).pc == call_pc && (why(d), strcmp(reason, "run to") == 0), "run to the call");
        c = cpu(d);
        arcdebug_command(d, "step 5", out, sizeof out);
        check(wait_stopped(d, 1, 2000) && cpu(d).instructions == c.instructions + 5, "`step 5` runs five instructions");
        check(strcmp((why(d), reason), "step") == 0, "and stops as 'step'");
        check(arcdebug_trace_read(d, tr, 16) >= 6 && tr[0].pc == cpu(d).pc, "the trace ring's newest entry is the PC");
        arcdebug_trace_enable(d, 0);
        arcdebug_trace_clear(d);
        check(arcdebug_trace_read(d, tr, 1) == 0, "trace clear");
    }

    /* ---- scanline and frame ---- */
    {
        arcdebug_cpu a, b;
        a = cpu(d);
        arcdebug_scanline(d, 1);
        check(wait_stopped(d, 1, 3000), "scanline stops");
        b = cpu(d);
        check(b.vpos == (a.vpos + 1) % 262, "scanline+1 advances the beam one line");
        check(strcmp((why(d), reason), "scanline") == 0, "reason 'scanline'");
        a = b;
        arcdebug_command(d, "scanline 7", out, sizeof out);
        check(wait_stopped(d, 1, 3000), "scanline 7 stops");
        b = cpu(d);
        check(b.vpos == (a.vpos + 7) % 262, "scanline+7 advances seven lines");
        a = b;
        arcdebug_frame(d, 1);
        check(wait_stopped(d, 1, 3000), "frame stops");
        b = cpu(d);
        check(b.frame == a.frame + 1 && strcmp((why(d), reason), "frame") == 0, "frame+1 advances one frame");
        a = b;
        arcdebug_command(d, "frame 3", out, sizeof out);
        check(wait_stopped(d, 1, 3000) && cpu(d).frame == a.frame + 3, "frame 3 advances three");
    }

    /* ---- breakpoints ---- */
    {
        arcdebug_line ln[4];

        check(arcdebug_breakpoint_add(d, ARCDEBUG_BP_EXEC, ARC_SPACE_PROG, 0xB0, 0xB0, "this bogus ((") == -1,
              "an unparsable condition is refused");
        check(arcdebug_breakpoint_add(d, 0, 0, 0, 0, "") == -1, "no type is refused");
        check(arcdebug_breakpoint_add(d, 8, 0, 0, 0, "") == -1, "an old IN breakpoint type is refused");
        check(arcdebug_breakpoint_add(d, ARCDEBUG_BP_READ, ARC_SPACE_UVI, 0, 0, "") == -1, "a data breakpoint in another space is refused");
        check(arcdebug_breakpoint_add(d, ARCDEBUG_BP_EXEC, ARC_SPACE_PROG, 0x8000, 0x8001, "") == -1, "outside PROG space is refused");
        check(arcdebug_breakpoint_toggle(d, 0x6B0) == 1 && arcdebug_breakpoint_check(d, 0x6B0), "toggle sets an execute breakpoint");
        n = arcdebug_disassemble(d, 0x6B0, ln, 1, NULL);
        check(n == 1 && ln[0].has_breakpoint, "the line shows it");
        check(arcdebug_breakpoint_toggle(d, 0x6B0) == 0 && !arcdebug_breakpoint_check(d, 0x6B0), "toggle again removes it");

        /* a false condition never stops; a true one does, on a UVI write CONFIG makes every frame */
        id = arcdebug_breakpoint_add(d, ARCDEBUG_BP_WRITE, ARC_SPACE_PROG, 0x1800, 0x1AFF, "sp == 31");
        arcdebug_resume(d);
        sleep_ms(400);
        check(!arcdebug_is_stopped(d), "a false condition never stops");
        check(stop_now(d), "stop");
        arcdebug_breakpoint_remove(d, id);
        id = arcdebug_breakpoint_add(d, ARCDEBUG_BP_WRITE, ARC_SPACE_PROG, 0x1800, 0x1AFF, "sp < 8 && frame > 5 && addr >= $1800");
        check(id > 0 && run_until_stop(d, 5000) && strncmp((why(d), reason), "write prog $1", 13) == 0,
              "a true condition stops on a UVI write");
        arcdebug_breakpoint_enable(d, id, 0);
        n = arcdebug_breakpoint_list(d, bps, 16);
        check(n == 1 && !bps[0].enabled && bps[0].hits >= 1 &&
              strcmp(bps[0].condition, "sp < 8 && frame > 5 && addr >= $1800") == 0, "disabled, with its hit count and condition");
        arcdebug_resume(d);
        sleep_ms(300);
        check(!arcdebug_is_stopped(d), "a disabled breakpoint never stops");
        check(stop_now(d), "stop");
        arcdebug_breakpoint_clear(d);

        /* execute over a range */
        id = arcdebug_breakpoint_add(d, ARCDEBUG_BP_EXEC, ARC_SPACE_PROG, 0x0000, 0x1FFF, "");
        check(id > 0 && run_until_stop(d, 5000) && cpu(d).pc < 0x2000 && strstr((why(d), reason), "breakpoint: exec") != NULL,
              "an execute breakpoint over a range");
        arcdebug_breakpoint_clear(d);
        /* with everything cleared, nothing stops */
        arcdebug_resume(d);
        sleep_ms(100);
        check(!arcdebug_is_stopped(d), "and breakpoints that are cleared stop nothing");
        check(stop_now(d), "stop");
    }

    /* ---- the UVI ---- */
    {
        arcdebug_uvi u;
        arcdebug_uvi_cell cells[ARCDEBUG_UVI_COLS * ARCDEBUG_UVI_MAX_ROWS];
        char name[64], text[200];
        arcdebug_hit hits[16];
        uint8_t save[0x300];
        int rows;

        arcdebug_uvi_get(d, &u);
        check(u.lines == 262 && (u.rows == 13 || u.rows == 26) && (u.cell_height == 8 || u.cell_height == 16) &&
              u.rows == (u.two_screens ? 26 : 13) && u.line >= 0 && u.line < 262 && u.vblank == (u.line >= 216),
              "the UVI snapshot: geometry and the beam");
        check(u.obj[0].index == 0 && u.obj[3].udc == 0x3B && u.obj[2].width == 8 &&
              u.obj[1].height == (u.obj[1].double_height ? 16 : 8) && u.obj[0].px == u.obj[0].x + 32 &&
              u.obj[0].reg_x == u.reg[0xF1] && u.obj[0].reg_y == u.reg[0xF0],
              "the four objects: position registers, UDC index, size");
        check(u.screen_colour == (u.reg[0x1F9] & 7) && u.char_colour == ((u.reg[0x1F9] >> 3) & 7) &&
              u.alt_screen_colour == (u.reg[0x1F8] & 7), "the colours decode from $19F8/$19F9");
        check(memcmp(u.chars[0x38], u.reg + 0x180, 8) == 0 && memcmp(u.chars[0x3F], u.reg + 0x1B8, 8) == 0,
              "the glyph table's UDCs are the $1980 rows");
        rows = arcdebug_uvi_cells(d, cells);
        check(rows == u.rows, "the character screen has the snapshot's row count");
        {
            int ok = 1;
            for (i = 0; i < rows * ARCDEBUG_UVI_COLS; i++) {
                const int row = i / 16, col = i % 16;
                const uint8_t code = u.reg[row < 13 ? row * 16 + col : 0x200 + (row - 13) * 16 + col];
                if (cells[i].code != code || cells[i].glyph != (code & 0x3F) || cells[i].fg > 7 || cells[i].bg > 7)
                    ok = 0;
            }
            check(ok, "every cell: code, glyph, colours");
        }
        arcdebug_uvi_describe(d, 0xF0, name, sizeof name, text, sizeof text);
        check(strcmp(name, "OBJ0_Y") == 0 && strstr(text, "y="), "register names and text");
        arcdebug_uvi_describe(d, 0x1F9, name, sizeof name, text, sizeof text);
        check(strcmp(name, "PAL1") == 0 && strstr(text, "screen"), "PAL1 decodes");
        arcdebug_uvi_describe(d, 0x1234, name, sizeof name, text, sizeof text);
        check(name[0] != '\0', "an offset past the window folds to a register");
        check(strcmp(arcdebug_uvi_colour_name(0), "white") == 0 && strcmp(arcdebug_uvi_colour_name(7), "black") == 0,
              "colour names");

        /* put object 0 and 1 in known places with known images (restored below) */
        arcdebug_read(d, ARC_SPACE_UVI, 0, save, 0x300);
        arcdebug_uvi_write(d, 0xF0, 0xCE);              /* y = (0xCE ^ 0xFF) + 1 = 50 */
        arcdebug_uvi_write(d, 0xF1, 83);                /* x = 83 - 43 = 40 */
        arcdebug_uvi_write(d, 0xF2, 0xCE);
        arcdebug_uvi_write(d, 0xF3, 83);
        for (i = 0; i < 8; i++) {
            arcdebug_uvi_write(d, 0x180 + (unsigned)i, 0xFF);
            arcdebug_uvi_write(d, 0x188 + (unsigned)i, 0x81);
        }
        arcdebug_uvi_write(d, 0x1FB, 0xC0 | (5 << 3) | 6);   /* obj0 red 8x8, obj1 blue 8x8 */
        arcdebug_uvi_get(d, &u);
        check(u.obj[0].x == 40 && u.obj[0].y == 50 && u.obj[0].px == 72 && u.obj[0].py == 50 && u.obj[0].colour == 5 &&
              u.obj[1].colour == 6 && !u.obj[0].double_height && u.obj[0].visible && u.obj[0].shape[3] == 0xFF &&
              u.obj[1].shape[3] == 0x81, "an object placed by register writes decodes: position, colour, size, image");

        /* the views */
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FRAME, ARCDEBUG_UVI_L_ALL, 0, 1, NULL, 0, &w, &h) &&
              w == ARCSESSION_FB_WIDTH && h == ARCSESSION_FB_HEIGHT, "FRAME: asking for the size");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FRAME, ARCDEBUG_UVI_L_ALL, 0, 2, NULL, 0, &w, &h) &&
              w == 2 * ARCSESSION_FB_WIDTH && h == 2 * ARCSESSION_FB_HEIGHT, "at scale 2 it is twice that");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FRAME, ARCDEBUG_UVI_L_ALL, 0, 1, img, 100, &w, &h) == 0,
              "a buffer too small draws nothing");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FRAME, ARCDEBUG_UVI_L_ALL, 0, 1, img,
                                sizeof img / sizeof img[0], &w, &h) && w == ARCSESSION_FB_WIDTH, "FRAME draws");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FRAME, ARCDEBUG_UVI_L_ALL, 0, 2, img2,
                                sizeof img2 / sizeof img2[0], &w, &h) && w == 2 * ARCSESSION_FB_WIDTH, "FRAME at scale 2 draws");
        {
            const int W = ARCSESSION_FB_WIDTH, H = ARCSESSION_FB_HEIGHT;
            int same = 1;
            for (int y = 0; y < H && same; y += 7)
                for (int x = 0; x < W && same; x += 5) {
                    const uint32_t p = img[y * W + x];
                    same = img2[(2 * y) * 2 * W + 2 * x] == p && img2[(2 * y) * 2 * W + 2 * x + 1] == p &&
                           img2[(2 * y + 1) * 2 * W + 2 * x] == p && img2[(2 * y + 1) * 2 * W + 2 * x + 1] == p;
                }
            check(same, "the scaled frame is the frame, each pixel a 2x2 block");
        }
        {
            static uint32_t chars_only[ARCDEBUG_UVI_VIEW_MAX_PIXELS], with_objects[ARCDEBUG_UVI_VIEW_MAX_PIXELS],
                            boxed[ARCDEBUG_UVI_VIEW_MAX_PIXELS];
            const int W = ARCSESSION_FB_WIDTH;
            int differ = 0, boxes = 0;

            check(arcdebug_uvi_view(d, ARCDEBUG_UVI_RENDER, ARCDEBUG_UVI_L_CHARS, 0, 1, chars_only, ARCDEBUG_UVI_VIEW_MAX_PIXELS, &w, &h) &&
                  arcdebug_uvi_view(d, ARCDEBUG_UVI_RENDER, ARCDEBUG_UVI_L_ALL, 0, 1, with_objects, ARCDEBUG_UVI_VIEW_MAX_PIXELS, &w, &h),
                  "RENDER with layer masks");
            for (i = 0; i < W * ARCSESSION_FB_HEIGHT; i++)
                if (chars_only[i] != with_objects[i]) differ++;
            check(differ > 0, "the objects are drawn on the object layer");
            check(with_objects[53 * W + 75] != chars_only[53 * W + 75] || with_objects[(50 + 4) * W + 72 + 4] != chars_only[(50 + 4) * W + 72 + 4],
                  "object 1 (hollow, over object 0) shows its colour over the solid one");
            check(arcdebug_uvi_view(d, ARCDEBUG_UVI_RENDER, ARCDEBUG_UVI_L_ALL,
                                    ARCDEBUG_UVI_F_BOXES | ARCDEBUG_UVI_F_GRID | ARCDEBUG_UVI_F_BEAM | ARCDEBUG_UVI_F_COLLIDE,
                                    1, boxed, ARCDEBUG_UVI_VIEW_MAX_PIXELS, &w, &h) && w == W, "RENDER with every overlay");
            for (i = 0; i < W * ARCSESSION_FB_HEIGHT; i++)
                if (boxed[i] != with_objects[i]) boxes++;
            check(boxes > 50, "and the overlays draw");
            check(boxed[(50 - 1) * W + 72 - 1] != with_objects[(50 - 1) * W + 72 - 1] ||
                  boxed[(50 - 1) * W + 72] != with_objects[(50 - 1) * W + 72], "an object's box is one pixel outside it");
        }
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_PROGRESS, 0, ARCDEBUG_UVI_F_BEAM, 1, img, sizeof img / sizeof img[0], &w, &h) &&
              w == ARCSESSION_FB_WIDTH && h == ARCSESSION_FB_HEIGHT, "PROGRESS (the frame to the beam) draws");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FONT, 0, 0, 2, NULL, 0, &w, &h) && w == 16 * 18 && h == 4 * 18, "FONT sheet size");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_FONT, 0, 0, 2, img, sizeof img / sizeof img[0], &w, &h) && w == 16 * 18,
              "FONT draws");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_OBJECTS, 0, 0, 3, img, sizeof img / sizeof img[0], &w, &h) &&
              w == 4 * 24 + 3 * 6 && h == 48, "OBJECTS draws four images side by side");
        check(arcdebug_uvi_view(d, ARCDEBUG_UVI_PALETTE, 0, 0, 1, img, sizeof img / sizeof img[0], &w, &h) && w == 128 && h == 16,
              "PALETTE: 8 cells of 16");
        {
            int distinct = 0;
            for (i = 1; i < 8; i++)
                if (img[i * 16] != img[0]) distinct++;
            check(distinct == 7, "the palette's colours differ");
        }
        check(!arcdebug_uvi_view(d, 99, 0, 0, 1, img, sizeof img / sizeof img[0], &w, &h), "an unknown view is refused");

        /* a thing at a time */
        check(arcdebug_uvi_thing(d, ARCDEBUG_UVI_OBJECT, 0, 2, 0xFF000000u, img, 1024, &w, &h) && w == 16 && h == 16 &&
              img[0] == uvi_colours[5], "object 0, 2x, in its colour");
        {
            int lit = 0, bgp = 0;
            arcdebug_uvi_thing(d, ARCDEBUG_UVI_OBJECT, 1, 2, 0xFF000000u, img, 1024, &w, &h);
            for (i = 0; i < w * h; i++) { if (img[i] == 0xFF000000u) bgp++; else lit++; }
            check(lit > 0 && bgp > 0, "object 1's image has its colour on its lit pixels and the background around");
        }
        check(arcdebug_uvi_thing(d, ARCDEBUG_UVI_OBJECT, 0, 3, 0, NULL, 0, &w, &h) && w == 24 && h == 24, "an object's size");
        check(arcdebug_uvi_thing(d, ARCDEBUG_UVI_GLYPH, 12, 2, 0, img, 1024, &w, &h) && w == 16 && h == 16, "a glyph");
        check(arcdebug_uvi_thing(d, ARCDEBUG_UVI_GLYPH, 63, 1, 0, img, 1024, &w, &h) && w == 8, "the last UDC");
        check(!arcdebug_uvi_thing(d, ARCDEBUG_UVI_OBJECT, 0, 2, 0, img, 4, &w, &h), "a buffer too small");
        check(!arcdebug_uvi_thing(d, 7, 0, 2, 0, img, 1024, &w, &h), "an unknown kind");

        /* hit test: object 1 is above object 0, both above the character cell under them */
        n = arcdebug_uvi_hit_test(d, 72, 50, hits, 16);
        check(n >= 2 && hits[0].kind == ARCDEBUG_UVI_HIT_OBJECT && hits[0].index == 1 &&
              hits[1].kind == ARCDEBUG_UVI_HIT_OBJECT && hits[1].index == 0,
              "overlapping objects come back topmost first: object 1, object 0 ...");
        {
            int cell = 0;
            for (i = 2; i < n; i++)
                if (hits[i].kind == ARCDEBUG_UVI_HIT_CELL) cell = hits[i].index;
            check(n == 3 && cell == ((50 - u.first_line) / u.cell_height) * 16 + (72 - 32 - u.shift) / 8, "... then the character cell under them");
        }
        n = arcdebug_uvi_hit_test(d, 72 + 3, 50 + 3, hits, 16);
        check(n >= 1 && hits[0].index == 0 && hits[0].kind == ARCDEBUG_UVI_HIT_OBJECT,
              "object 1's hollow middle lets object 0 show through");
        check(arcdebug_uvi_hit_test(d, -40, -40, hits, 16) == 0 && arcdebug_uvi_hit_test(d, 400, 400, hits, 16) == 0,
              "and nothing outside the picture");
        check(arcdebug_uvi_hit_test(d, 72, 50, hits, 0) == 0, "max 0 asks for nothing");
        check(arcdebug_uvi_hit_test(d, 5, 5, hits, 16) == 0, "the border outside the 128 pixels has no cell");

        /* edits show, and are undone */
        for (i = 0xF0; i < 0xF8; i++) arcdebug_uvi_write(d, (unsigned)i, save[i]);
        for (i = 0x180; i < 0x190; i++) arcdebug_uvi_write(d, (unsigned)i, save[i]);
        arcdebug_uvi_write(d, 0x1FB, save[0x1FB]);
        arcdebug_uvi_get(d, &u);
        check(memcmp(u.reg + 0xF0, save + 0xF0, 8) == 0 && memcmp(u.reg + 0x180, save + 0x180, 16) == 0 &&
              u.reg[0x1FB] == save[0x1FB], "the UVI as it was");
    }

    /* ---- I/O, the cartridge ---- */
    {
        arcdebug_io io;
        arcdebug_cart cart;

        arcdebug_io_get(d, &io);
        check(io.machine == ARC_MACHINE_ARCADIA && !io.is_pal && io.cart_type == ARC_CART_FUJINET, "I/O: the NTSC machine, the FujiNet cartridge");
        check(io.keys == 0 && io.sticks[0] == 0 && io.sticks[1] == 0 && !io.reset_held && io.panel == 0,
              "nothing held");
        check((io.pad[0] & 0xF0) == 0xF0 && io.sense == (cpu(d).vpos >= 216), "keypad columns idle, SENSE follows the beam");
        check(io.audio_samples > 10000, "audio samples have been made");

        arcdebug_cart_get(d, &cart);
        check(cart.present && cart.type == ARC_CART_FUJINET && strcmp(cart.type_name, "FujiNet") == 0, "cartridge: the FujiNet");
        check(cart.mailbox && !cart.booted_game && cart.live_size > 0 && cart.live_crc != 0,
              "running the baked CONFIG, mailbox live");
        check(cart.mailbox_page[0x109] == 'F' && cart.mailbox_page[0x10A] == 'N' && cart.mailbox_page[0x10B] == FN_PROTO_VER,
              "the mailbox page carries 'FN' and the protocol version at $2C09");
        printf("      CONFIG: %u bytes, %d APPBANK pages, app_live %d\n", cart.live_size, cart.nbanks, cart.app_live);
        arcdebug_cart_info(d, out, sizeof out);
        check(strncmp(out, "FujiNet cartridge: CONFIG", 25) == 0, "the cartridge's one line");
        check(arcdebug_fujinet_state(d, out, sizeof out) > 0 && strstr(out, "Link") && strstr(out, "Mailbox"), "the FujiNet state text");
#ifndef _WIN32
        if (fake_up) {
            int waited = 0;
            while (!cart.link_up && waited < 5000) { sleep_ms(50); waited += 50; arcdebug_cart_get(d, &cart); }
            check(cart.link_up && !cart.link_off, "the cartridge's link comes up to the stand-in FujiNet");
        }
#endif
    }

    /* ---- the FujiNet trace, from session start, attached or not ---- */
#ifndef _WIN32
    if (fake_up) {
        int waited = 0, dirs[2] = { 0, 0 };
        /* CONFIG asks the FujiNet things as it boots; the trace has recorded since the start */
        arcdebug_resume(d);
        n = 0;
        while (waited < 8000) {
            n = arcdebug_fujinet_trace(d, pk, 512);
            dirs[0] = dirs[1] = 0;
            for (i = 0; i < n; i++) dirs[pk[i].to_host ? 1 : 0] = 1;
            if (dirs[0] && dirs[1]) break;
            sleep_ms(50); waited += 50;
        }
        check(dirs[0] && dirs[1], "the trace saw packets both ways on the cartridge's own socket");
        if (n > 0)
            check(strstr(pk[0].text, "Arcadia 2001") && pk[0].nbytes >= 6 && pk[0].serial > 0, "described, with the FujiBus header");
        arcdebug_fujinet_trace_clear(d);
        check(arcdebug_fujinet_trace(d, pk, 512) == 0, "clear empties the ring");
        check(stop_now(d), "stop again");
    } else
        printf("SKIP: no loopback listener, the FujiNet trace is untested here\n");
#else
    printf("SKIP: the loopback stand-in FujiNet is POSIX only\n");
#endif

    /* ---- symbols: every format, and the built-in tables ---- */
    {
        snprintf(path, sizeof path, "%s/t.map", tmp);
        write_text(path, "Symbols in Segment NOTHING\nR8                 Int    8   -1  1 0\n\n"
                         "Symbols in Segment CODE\nloop               Int    42D -1  1 0\n");
        check(arcdebug_load_symbols(d, path, msg, sizeof msg) == 1, "an AS -g MAP .map loads (code only)");
        check(arcdebug_label_address(d, "loop") == 0x42D && arcdebug_label_address(d, "R8") == -1, "its labels resolve");
        snprintf(path, sizeof path, "%s/t.lst", tmp);
        write_text(path, "  Symbol Table (* = unused):\n  --------------------------\n\n"
                         " RA :                            A - | *isrw :                     451 C |\n"
                         " up :                          43B C |\n");
        check(arcdebug_load_symbols(d, path, msg, sizeof msg) == 2, "an AS listing's symbol table loads");
        check(arcdebug_label_address(d, "isrw") == 0x451, "unused labels too");
        snprintf(path, sizeof path, "%s/t.sym", tmp);
        write_text(path, "; WLA-DX\n[labels]\n00:0430 bump\n");
        check(arcdebug_load_symbols(d, path, msg, sizeof msg) == 1 && arcdebug_label_address(d, "bump") == 0x430,
              "a WLA-DX .sym loads");
        snprintf(path, sizeof path, "%s/t.txt", tmp);
        write_text(path, "2811 key1\n");
        check(arcdebug_load_symbols(d, path, msg, sizeof msg) == 1, "a plain list loads");
        arcdebug_address_label(d, 0x2811, out, sizeof out);
        check(strcmp(out, "key1") == 0, "and the address (past 4K) finds its label");
        check(arcdebug_load_symbols(d, "/nonexistent/file.map", msg, sizeof msg) == -1, "a missing file is refused");

        n = arcdebug_disassemble(d, 0x42D, lines, 1, NULL);
        check(n == 1 && strcmp(lines[0].label, "loop") == 0, "loaded labels show in the disassembly");
        arcdebug_set_label(d, 0x400, "myentry");
        arcdebug_address_label(d, 0x400, out, sizeof out);
        check(strcmp(out, "myentry") == 0, "a user label");
        arcdebug_set_label(d, 0x400, NULL);
        arcdebug_address_label(d, 0x400, out, sizeof out);
        check(out[0] == '\0', "removed again");

        snprintf(path, sizeof path, "%s/game.bin", tmp);
        arcdebug_set_cart_path(d, path);
        check(arcdebug_load_symbols(d, NULL, msg, sizeof msg) == -1, "no symbol file beside the cartridge");
        snprintf(path, sizeof path, "%s/game.sym", tmp);
        write_text(path, "[labels]\n00:0440 beside\n");
        check(arcdebug_load_symbols(d, NULL, msg, sizeof msg) == 1 && arcdebug_label_address(d, "beside") == 0x440,
              "symbols beside the cartridge load with no path");
        arcdebug_set_cart_path(d, "");
    }

    /* ---- the trace ring ---- */
    {
        arcdebug_trace tr[16];
        arcdebug_cpu a = cpu(d);
        arcdebug_trace_clear(d);
        arcdebug_trace_enable(d, 1);
        for (i = 0; i < 10; i++) {
            arcdebug_step(d);
            wait_stopped(d, 1, 2000);
        }
        n = arcdebug_trace_read(d, tr, 16);
        check(n == 10, "the trace holds one entry per boundary stepped to");
        c = cpu(d);
        check(n >= 3 && tr[0].pc == c.pc && tr[0].cycles == c.cycles && tr[0].cycles > tr[1].cycles && tr[1].cycles > tr[2].cycles,
              "newest first, cycles falling, the newest being the PC");
        check(n == 10 && tr[9].cycles > a.cycles && tr[9].cycles <= a.cycles + 12, "the oldest is the first step's result");
        {
            uint8_t b = 0;
            arcdebug_read(d, ARC_SPACE_PROG, tr[3].pc, &b, 1);
            check(tr[3].bytes[0] == b && tr[3].len == dasm2650_length(b) && tr[3].vpos >= 0 && tr[3].vpos < 262 &&
                  tr[3].hpos >= 0 && tr[3].hpos < ARCSESSION_FB_WIDTH, "an entry: opcode, length and beam");
        }
        arcdebug_trace_enable(d, 0);
        arcdebug_trace_clear(d);
    }

    /* ---- Save ---- */
    {
        static const struct { const char *kind; long size; } kinds[] = {
            { "mailbox", 0x500 }, { "uvi", 0x300 }, { "font", 512 }, { "cart", ARCMAP_STORE },
        };
        for (i = 0; i < (int)(sizeof kinds / sizeof kinds[0]); i++) {
            char what[96];
            snprintf(path, sizeof path, "%s/save.%s", tmp, kinds[i].kind);
            snprintf(what, sizeof what, "save %s writes %ld bytes", kinds[i].kind, kinds[i].size);
            check(arcdebug_save(d, kinds[i].kind, path, msg, sizeof msg) == 0 && file_size(path) == kinds[i].size, what);
            show("%s", msg);
        }
        snprintf(path, sizeof path, "%s/save.ppm", tmp);
        {
            char hdr[32];
            const int hl = snprintf(hdr, sizeof hdr, "P6\n%d %d\n255\n", ARCSESSION_FB_WIDTH, ARCSESSION_FB_HEIGHT);
            check(arcdebug_save(d, "screen", path, msg, sizeof msg) == 0 &&
                  file_size(path) == hl + ARCSESSION_FB_WIDTH * ARCSESSION_FB_HEIGHT * 3, "save screen (a PPM of the frame)");
        }
        snprintf(path, sizeof path, "%s/save.regs", tmp);
        check(arcdebug_save(d, "regs", path, msg, sizeof msg) == 0 && file_size(path) > 300, "save regs");
        snprintf(path, sizeof path, "%s/save.dis", tmp);
        check(arcdebug_save(d, "dis", path, msg, sizeof msg) == 0 && file_size(path) > 8000, "save dis (the cartridge as code)");
        check(arcdebug_save(d, "nonsense", path, msg, sizeof msg) == -1 && strstr(msg, "Unknown"), "an unknown kind is refused");
        check(arcdebug_save(d, "uvi", "/nonexistent/dir/x", msg, sizeof msg) == -1 && strstr(msg, "Cannot"), "an unwritable path is refused");
    }

    /* ---- the prompt ---- */
    arcdebug_cpu_set(d, ARC_REG_R0, 0x66);
    arcdebug_command(d, "print r0", out, sizeof out);
    check(strstr(out, "$66") && strstr(out, "102") && strstr(out, "%01100110"), "print evaluates a register");
    arcdebug_cpu_set(d, ARC_REG_R0, saved.r0);
    arcdebug_command(d, "print [0] == p[0] && c[0] == p[0] && u[$F0] >= 0 && p[$2C09] == $46", out, sizeof out);
    check(strstr(out, "1  "), "[addr] reads PROG space; p[] u[] c[] the three spaces");
    arcdebug_command(d, "print UVI_PAL1 + 1", out, sizeof out);
    check(strstr(out, "$19FA"), "labels in expressions");
    arcdebug_command(d, "print r1' + vpos * 0 + (frame > 0) + (cycles > 0) + (hpos >= 0) + cc + ii + rs + s", out, sizeof out);
    check(strstr(out, "error") == NULL, "r1', vpos, hpos, frame, cycles, cc, ii, rs and s are names");
    arcdebug_command(d, "print nonesuch", out, sizeof out);
    check(strstr(out, "unknown name"), "an unknown name is an error");
    arcdebug_command(d, "print 1 +", out, sizeof out);
    check(strstr(out, "error"), "so is a syntax error");
    arcdebug_command(d, "print 3 << 2 | 1", out, sizeof out);
    check(strstr(out, "$0D"), "C precedence: 3 << 2 | 1 is 13");
    arcdebug_command(d, "mem 0 5", out, sizeof out);
    check(strstr(out, "prog space") && strstr(out, "0000 "), "mem dumps PROG space");
    arcdebug_command(d, "mem $2B00 4", out, sizeof out);
    check(strstr(out, "FN_R_DATA+00"), "mem prog names the mailbox bytes");
    arcdebug_command(d, "mem uvi $F0 8", out, sizeof out);
    check(strstr(out, "uvi space") && strstr(out, "OBJ0_Y") && strstr(out, "OBJ3_X"), "mem uvi describes the object registers");
    arcdebug_command(d, "mem cart $1C09 2", out, sizeof out);
    check(strstr(out, "cart space") && strstr(out, "46 4E"), "mem cart");
    arcdebug_command(d, "poke uvi $1B0 $11 $22", out, sizeof out);
    {
        uint8_t v[2];
        arcdebug_read(d, ARC_SPACE_UVI, 0x1B0, v, 2);
        check(v[0] == 0x11 && v[1] == 0x22 && strstr(out, "2 bytes"), "poke uvi");
        arcdebug_write(d, ARC_SPACE_UVI, 0x1B0, saved.r0 ? 0 : 0);
        arcdebug_write(d, ARC_SPACE_UVI, 0x1B1, 0);
    }
    arcdebug_command(d, "label $0431 latch", out, sizeof out);
    check(arcdebug_label_address(d, "latch") == 0x431, "label sets one");
    arcdebug_command(d, "label $0431", out, sizeof out);
    check(arcdebug_label_address(d, "latch") == -1, "and no name removes it");
    arcdebug_command(d, "disasm 0 3", out, sizeof out);
    check(strstr(out, "reset:") && strstr(out, "0000 "), "disasm");
    arcdebug_command(d, "regs", out, sizeof out);
    check(strstr(out, "PC=") && strstr(out, "bank 0") && strstr(out, "bank 1") && strstr(out, "return stack"), "regs");
    arcdebug_command(d, "uvi", out, sizeof out);
    check(strstr(out, "2637 UVI") && strstr(out, "$18F0") && strstr(out, "$19F9") && strstr(out, "colours"), "uvi");
    arcdebug_command(d, "objects", out, sizeof out);
    check(strstr(out, "object 0") && strstr(out, "object 3") && strstr(out, "shape"), "objects");
    arcdebug_command(d, "screen", out, sizeof out);
    check(strstr(out, " 0 ") && strstr(out, "12 ") && strstr(out, "block graphics"), "screen");
    arcdebug_command(d, "io", out, sizeof out);
    check(strstr(out, "Arcadia 2001 (NTSC)") && strstr(out, "FujiNet") && strstr(out, "SENSE"), "io");
    arcdebug_command(d, "keys", out, sizeof out);
    check(strstr(out, "keypad 1") && strstr(out, "stick 1") && strstr(out, "centred"), "keys");
    arcdebug_command(d, "cart", out, sizeof out);
    check(strstr(out, "FujiNet") && strstr(out, "image"), "cart");
    arcdebug_command(d, "mailbox", out, sizeof out);
    check(strstr(out, "ACKSEQ") && strstr(out, "2B00"), "mailbox");
    arcdebug_command(d, "trace on", out, sizeof out);
    arcdebug_command(d, "step 3", out, sizeof out);
    wait_stopped(d, 1, 2000);
    arcdebug_command(d, "trace 3", out, sizeof out);
    check(arcdebug_trace_enabled(d) && strstr(out, "R0=") && strstr(out, "PSL="), "trace lists instructions");
    arcdebug_command(d, "trace off", out, sizeof out);
    arcdebug_command(d, "trace clear", out, sizeof out);
    check(!arcdebug_trace_enabled(d), "trace off");
    arcdebug_command(d, "break $6B0", out, sizeof out);
    check(arcdebug_breakpoint_check(d, 0x6B0), "break sets a breakpoint");
    arcdebug_command(d, "bpw $18F0-$18F7 value == 0", out, sizeof out);
    arcdebug_command(d, "bpr $2B00-$2FFF", out, sizeof out);
    arcdebug_command(d, "bpw $2000", out, sizeof out);
    check(strstr(out, "will not fire"), "a write breakpoint outside the UVI says it cannot fire");
    arcdebug_command(d, "breaks", out, sizeof out);
    check(strstr(out, "$18F0-$18F7 if value == 0") && strstr(out, "$2B00-$2FFF") && strstr(out, "$06B0"), "breaks lists every class");
    show("%s", out);
    arcdebug_command(d, "bpw $18F0 ((", out, sizeof out);
    check(strstr(out, "error"), "a bad condition is refused");
    arcdebug_command(d, "bpr $9000", out, sizeof out);
    check(strstr(out, "out of range"), "an address past $7FFF is refused");
    arcdebug_command(d, "delete $6B0", out, sizeof out);
    check(!arcdebug_breakpoint_check(d, 0x6B0), "delete by address");
    n = arcdebug_breakpoint_list(d, bps, 16);
    check(n == 3, "three breakpoints remain");
    id = bps[0].id;
    snprintf(path, sizeof path, "disable #%d", id);
    arcdebug_command(d, path, out, sizeof out);
    arcdebug_breakpoint_list(d, bps, 16);
    check(!bps[0].enabled, "disable by id");
    snprintf(path, sizeof path, "enable %d", id);
    arcdebug_command(d, path, out, sizeof out);
    arcdebug_breakpoint_list(d, bps, 16);
    check(bps[0].enabled, "enable by (plain) id");
    snprintf(path, sizeof path, "delete #%d", id);
    arcdebug_command(d, path, out, sizeof out);
    check(arcdebug_breakpoint_list(d, bps, 16) == n - 1, "delete by id");
    arcdebug_command(d, "delete 999", out, sizeof out);
    check(strstr(out, "no breakpoint"), "an unknown id is said so");
    arcdebug_command(d, "clear", out, sizeof out);
    check(arcdebug_breakpoint_list(d, bps, 16) == 0, "clear removes them");
    arcdebug_command(d, "set r0 $42", out, sizeof out);
    check(cpu(d).r0 == 0x42 && strstr(out, "R0=42"), "set a register by name");
    arcdebug_command(d, "set rs 1", out, sizeof out);
    arcdebug_command(d, "set r1' $77", out, sizeof out);
    check(cpu(d).r[1][1] == 0x77 && cpu(d).rs == 1, "set r1' and rs");
    arcdebug_cpu_set(d, ARC_REG_PSL, saved.psl);
    arcdebug_cpu_set(d, ARC_REG_R0, saved.r0);
    arcdebug_command(d, "set bogus 1", out, sizeof out);
    check(strstr(out, "usage"), "set with an unknown register");
    arcdebug_command(d, "help", out, sizeof out);
    check(strstr(out, "runto") && strstr(out, "bpw") && strstr(out, "uvi") && strstr(out, "save"), "help lists the commands");
    arcdebug_command(d, "frobnicate", out, sizeof out);
    check(strstr(out, "unknown command"), "an unknown command");
    check(arcdebug_completions(d, "ru", out, sizeof out) == 2 && strstr(out, "run") && strstr(out, "runto"), "Tab completion");
    check(arcdebug_completions(d, "print r1", out, sizeof out) == 2 && strstr(out, "r1'") && strstr(out, "r1\n"), "registers complete");
    check(arcdebug_completions(d, "print UVI_OBJ0", out, sizeof out) == 2 && strstr(out, "UVI_OBJ0_Y") && strstr(out, "UVI_OBJ0_X"),
          "labels complete");
    check(arcdebug_completions(d, "mem c", out, sizeof out) == 1 && strstr(out, "cart"), "spaces complete");
    check(arcdebug_completions(d, "save r", out, sizeof out) == 1 && strstr(out, "regs"), "save kinds complete");
    if (call_pc >= 0) {
        snprintf(path, sizeof path, "runto $%04X", call_pc);
        arcdebug_command(d, path, out, sizeof out);
        check(wait_stopped(d, 1, 5000) && cpu(d).pc == call_pc, "the prompt's runto");
        arcdebug_command(d, "over", out, sizeof out);
        check(wait_stopped(d, 1, 5000) && cpu(d).pc == ((call_pc + 3) & 0x7fff), "the prompt's over");
    }
    arcdebug_command(d, "run", out, sizeof out);
    sleep_ms(50);
    check(!arcdebug_is_stopped(d), "the prompt's run");
    arcdebug_command(d, "stop", out, sizeof out);
    check(wait_stopped(d, 1, 3000), "the prompt's stop");

    /* ---- detach lets it run ---- */
    arcdebug_detach(d);
    check(!arcdebug_is_attached(d) && !arcdebug_is_stopped(d), "detached");
    {
        const uint64_t f0 = arc_host_frame_count();
        sleep_ms(200);
        check(arc_host_frame_count() > f0 + 3, "the machine runs at full speed again");
    }
    arcdebug_attach(d);
    check(wait_stopped(d, 1, 3000), "attaching again stops it");
    arcdebug_detach(d);

    /* ---- a power cycle with the debugger attached: a PAL machine with a game cartridge ---- */
    arcdebug_attach(d);
    check(wait_stopped(d, 1, 3000), "stopped");
    arcdebug_breakpoint_toggle(d, 0x002);
    arcdebug_power_down(d);
    arc_host_stop();
    check(arcdebug_machine() == NULL, "the host is stopped");
    c = cpu(d);
    check(c.cycles == 0 && !arcdebug_is_stopped(d), "no machine: a zero view again");
    {
        static uint8_t game[2048];
        build_game(game);
        ho.machine = ARC_MACHINE_ARCADIA_PAL;
        ho.cart = game;
        ho.cart_size = sizeof game;
        ho.cart_type = ARC_CART_STD;
        ho.cart_app = false;
        ho.boip_hostport = NULL;
        check(arc_host_start(&ho) == 0, "the host starts again with a game cartridge in the slot");
    }
    arcdebug_power_up(d);
    check(wait_stopped(d, 1, 3000), "the new machine stops in the debugger at once");
    if (cpu(d).pc != 0 || cpu(d).instructions != 0) {      /* the race described above */
        printf("note: the machine ran before the hook went in\n");
        arcdebug_cpu_set(d, ARC_REG_PC, 0);
    }
    check(cpu(d).pc == 0 && cpu(d).machine == ARC_MACHINE_ARCADIA_PAL, "on the reset vector of the PAL machine");
    check(arcdebug_breakpoint_check(d, 0x002), "the breakpoint survived the power cycle");
    check(run_until_stop(d, 5000) && cpu(d).pc == 0x002 && strstr((why(d), reason), "breakpoint: exec $0002"),
          "and fires on the new machine");
    arcdebug_breakpoint_clear(d);
    game_tests(d, tmp);

    arcdebug_detach(d);
    sleep_ms(100);
    check(arc_host_frame_count() > 2, "and the new machine runs");

    arcdebug_power_down(d);
    arc_host_stop();
    arcdebug_free(d);
#ifndef _WIN32
    if (fake_up) {
        struct sockaddr_in a;
        int s = socket(AF_INET, SOCK_STREAM, 0);

        atomic_store(&fake.stop, 1);    /* a dial lets its accept() go */
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons((unsigned short)fake.port);
        if (s >= 0) {
            connect(s, (struct sockaddr *)&a, sizeof a);
            close(s);
        }
        pthread_join(fake_th, NULL);
        close(fake.listen_fd);
    }
#endif
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
