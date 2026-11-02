/*
 * arcdebug -- the debugger contract: an MCS-48 / 8244 debugger engine over the
 * core (core/debugger/) behind a C API the four native debugger windows share.
 * Same shape as the Studio II and SMS siblings' debug headers, so the windows
 * carry over; the engine is that one with the COSMAC and the Pixie traded for
 * the 8048, the 8244/8245 VDC (and the G7400's EF9340, The Voice's SP0256) and
 * the FujiNet cartridge's mailbox.
 *
 * The engine only engages while a window holds it attached:
 * arcdebug_attach() when the window opens -- which also stops the machine, as
 * on every sibling -- and arcdebug_detach() when it closes, which lets the
 * machine run on at full speed with no hook installed. The FujiNet trace is
 * the exception: it records from session start, so it shows what happened
 * before anyone opened a window.
 *
 * Calls are safe from the UI thread at any time. Inspection is meant for the
 * stopped state; while running it answers with whatever is current.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARCDEBUG_H
#define ARCDEBUG_H

#include <stddef.h>
#include <stdint.h>

#include "arcsession.h"
#include "i8244.h"      /* the VDC snapshot, layers and flags (core/arc) */
#include "ef9340.h"     /* the G7400's text chip snapshot */
#include "sp0256.h"     /* The Voice's chip state */

#ifdef __cplusplus
extern "C" {
#endif

/* Lazily created; lives as long as the session. */
arcdebug *arcdebug_get(arcsession *s);

/* ---- attach / stop / go ---------------------------------------------------- */
/* Engage the engine and stop at the next instruction. Idempotent. */
void arcdebug_attach(arcdebug *d);
/* Disengage and let the machine run. Breakpoints are kept for the next
 * attach. */
void arcdebug_detach(arcdebug *d);
int  arcdebug_is_attached(arcdebug *d);

int  arcdebug_is_stopped(arcdebug *d);
void arcdebug_stop(arcdebug *d);         /* break at the next instruction */
void arcdebug_resume(arcdebug *d);       /* "run" */
/* Why the machine last stopped ("stopped", "breakpoint: exec $0412",
 * "write xdata $E3 = $12", "step", "frame", ...) and the PC, -1 if none.
 * Returns length. */
int  arcdebug_stop_reason(arcdebug *d, char *dst, int dstsz, int *address);
/* Bumped whenever a command ran or the machine stopped/resumed, so a window
 * knows when to refresh. */
unsigned arcdebug_generation(arcdebug *d);

/* ---- the prompt ------------------------------------------------------------ */
/* Run one command; "help" lists them: step [n], over, out, frame [n],
 * scanline [n], run, stop, runto <addr>, break <addr> [cond], bpr/bpw
 * <addr>[-<end>] [cond] (program data), bpxr/bpxw (external data),
 * bpir/bpiw (internal RAM), bpin/bpout <p1|p2|bus> [cond], delete <id|addr>,
 * enable/disable <id>, clear, breaks, print <expr>, regs, set <reg> <expr>,
 * mem <space> <addr> [len] (prog, xdata, iram, vdc), poke <space> <addr>
 * <val...>, vdc, objects, ef, voice, io, keys, cart, mailbox, label <addr>
 * <name>, labels [file], disasm [addr] [n], trace [on|off|n], save <kind>
 * <file>. Expressions: numbers ($0400, 0x400, 400h, 1024), registers (a pc
 * psw sp r0-r7 r0'-r7' t cnt p1 p2 bus c ac f0 f1 bs mb), [addr] program-byte
 * reads and the spaces' x[addr] i[addr] v[addr], labels, vpos hpos frame
 * cycles, and in a breakpoint condition `value` and `addr` for the access;
 * C operators with C precedence. Output text into dst; returns its length. */
int  arcdebug_command(arcdebug *d, const char *command, char *dst, int dstsz);
/* Completions for a prefix (commands, registers and labels), one per line.
 * Returns the count. */
int  arcdebug_completions(arcdebug *d, const char *prefix, char *dst, int dstsz);

/* ---- stepping shortcuts (the toolbar) -------------------------------------- */
void arcdebug_step(arcdebug *d);          /* F7: one instruction */
/* F8: over a CALL (run until the return address is reached again, with the
 * stack back where it was) */
void arcdebug_step_over(arcdebug *d);
/* Shift+F8: to the routine's RET / RETR */
void arcdebug_step_out(arcdebug *d);
void arcdebug_scanline(arcdebug *d, int n);
void arcdebug_frame(arcdebug *d, int n);
/* Run until PC == addr (12-bit program address). */
void arcdebug_run_to(arcdebug *d, uint16_t addr);

/* ---- CPU --------------------------------------------------------------------- */
typedef struct {
    int a;
    int pc;                                /* 12 bits: the instruction about to run */
    int mb;                                /* the A11 latch (SEL MB0/MB1) */
    int psw;                               /* raw: C AC F0 BS 1 SP2 SP1 SP0 as the 8048 keeps it */
    int cy, ac, f0, f1, bs, sp;            /* decoded; bs = register bank in use, sp = 0..7 */
    uint8_t iram[64];                      /* the 8048's internal RAM (both register banks first) */
    uint8_t r[2][8];                       /* R0-R7 of bank 0 and bank 1 */
    int timer, prescaler;                  /* the timer/counter and its /32 prescaler */
    int timer_mode;                        /* 0 stopped, 1 timer, 2 event counter (T1) */
    int tf;                                /* the timer-overflow flag (JTF) */
    int tcnti_en, ext_int_en;              /* EN TCNTI / EN I */
    int in_interrupt;                      /* an interrupt routine is running */
    int irq_pending, timer_irq_pending;
    int t0, t1, int_line;                  /* the input pins */
    int p1, p2, bus;                       /* the output latches */
    uint64_t cycles;                       /* machine cycles since power-on */
    uint64_t instructions;
    int vpos, hpos;                        /* where the beam is (NTSC/PAL lines, pixels) */
    uint32_t frame;
    int machine;                           /* arc_machine */
} arcdebug_cpu;
void arcdebug_cpu_get(arcdebug *d, arcdebug_cpu *out);
typedef enum {
    ARC_REG_A = 0, ARC_REG_PC, ARC_REG_PSW, ARC_REG_SP, ARC_REG_T, ARC_REG_CNT,
    ARC_REG_P1, ARC_REG_P2, ARC_REG_BUS,
    ARC_REG_C, ARC_REG_AC, ARC_REG_F0, ARC_REG_F1, ARC_REG_BS, ARC_REG_MB,
    ARC_REG_R0,            /* ... ARC_REG_R0 + 7: the bank in use */
    ARC_REG_R0_B1 = ARC_REG_R0 + 8,   /* ... + 7: bank 1 */
    ARC_REG_COUNT = ARC_REG_R0_B1 + 8
} arcdebug_reg;
void arcdebug_cpu_set(arcdebug *d, int reg, int value);

/* ---- memory ------------------------------------------------------------------- */
/* The 8048's address spaces. Reads have no side effects (no mailbox hotspot,
 * no VDC read clearing a collision flag). */
typedef enum {
    ARC_SPACE_PROG = 0,   /* 12-bit: BIOS or stand-in $000-$3FF, the cartridge $400-$FFF */
    ARC_SPACE_XDATA,      /* external data $00-$FF: 128 bytes of RAM, the cart's registers, the VDC */
    ARC_SPACE_IRAM,       /* internal RAM $00-$3F */
    ARC_SPACE_VDC         /* the 8244's register file $00-$FF, raw */
} arcdebug_space;
int  arcdebug_read(arcdebug *d, int space, uint16_t addr, uint8_t *dst, int n);
/* A debugger edit: RAM takes it; a page the cartridge serves takes it in the
 * cartridge's own buffer (the image or the mailbox page), never as a hotspot;
 * the VDC takes it with MAME's data masks and ignores the write lockouts. */
void arcdebug_write(arcdebug *d, int space, uint16_t addr, uint8_t value);

/* ---- disassembly -------------------------------------------------------------- */
typedef struct {
    uint16_t address;
    int is_pc;
    int has_breakpoint;
    int length;
    char bytes[16];
    char label[48];
    char disasm[64];
    char comment[64];        /* a branch target's label, a BIOS entry's name, a mailbox register */
} arcdebug_line;
/* Up to `max` lines starting at `addr` (program space). Returns the count;
 * *pc_line is the index of the PC's line in `out`, -1 when not among them. */
int  arcdebug_disassemble(arcdebug *d, uint16_t addr, arcdebug_line *out,
                              int max, int *pc_line);
/* The address `rows` disassembly rows before (negative) or after `addr` --
 * for scrolling, and for "Follow PC" (PC a third of the way down). Backwards
 * is a best guess. */
int  arcdebug_row_address(arcdebug *d, uint16_t addr, int rows);
/* Address of a label, or -1. Label of an address into dst (may be empty). */
int  arcdebug_label_address(arcdebug *d, const char *label);
int  arcdebug_address_label(arcdebug *d, uint16_t addr, char *dst, int dstsz);
int  arcdebug_set_label(arcdebug *d, uint16_t addr, const char *label);
/* Load a symbol file: Macroassembler AS (-g MAP's .map, or the -L listing's
 * symbol table), WLA-DX .sym, or a plain "AAAA name" list. With path NULL,
 * <cart>.map/.lst/.sym next to the opened cartridge. Built-in labels: the BIOS
 * entry points (readkey, getjoystick, ...), the interrupt vectors and the
 * FujiNet mailbox registers and status page. Message into msg; returns labels
 * loaded or -1. */
int  arcdebug_load_symbols(arcdebug *d, const char *path, char *msg, int msgsz);

/* ---- video: the 8244/8245 and the G7400's EF9340 -------------------------------- */
/* Everything the chip knows: registers decoded, the four sprites, twelve
 * singles and four quads with their positions, the grid, collisions, sound
 * (i8244.h). */
void arcdebug_vdc_get(arcdebug *d, i8244_snapshot *out);
/* A register's name and a decoded value line (live). */
void arcdebug_vdc_describe(arcdebug *d, unsigned offset, char *name, size_t name_n,
                          char *text, size_t text_n);
/* A debugger store into the VDC (the machine must be stopped). */
void arcdebug_vdc_write(arcdebug *d, unsigned offset, uint8_t value);

/* The VDC views, as XRGB images (every one ends with the pixel size in *w,*h;
 * dst NULL asks for the size only). Returns 1 when drawn. layers is I8244_L_*
 * and flags I8244_F_* (boxes, collisions, overlaps, ...).
 *   FRAME:    the last frame the machine finished (what the display shows);
 *   RENDER:   the CURRENT register state re-rendered with the chosen layers
 *             and overlays (works while stopped, whatever the beam is doing);
 *   PROGRESS: the frame as far as the beam has got, the rest dimmed, with
 *             I8244_F_BEAM a cross-hair at the beam;
 *   FONT:     the 512-byte character generator as 64 glyphs;
 *   SHAPES:   the four sprite shape tables, in their colours;
 *   PALETTE:  the 16 colours. */
typedef enum {
    ARCDEBUG_VDC_FRAME = 0, ARCDEBUG_VDC_RENDER, ARCDEBUG_VDC_PROGRESS,
    ARCDEBUG_VDC_FONT, ARCDEBUG_VDC_SHAPES, ARCDEBUG_VDC_PALETTE
} arcdebug_vdc_view_kind;
#define ARCDEBUG_VDC_VIEW_MAX_PIXELS (I8244_WIDTH * I8244_HEIGHT)
int  arcdebug_vdc_view(arcdebug *d, int view, unsigned layers, unsigned flags,
                      int scale, uint32_t *dst, size_t cap, int *w, int *h);
/* One object as the chip draws it, for the tables: kind 0 sprite (0..3), 1
 * single (0..11), 2 quad member (0..15), 3 font glyph (0..63). */
int  arcdebug_vdc_object(arcdebug *d, int kind, int index, int scale, uint32_t bg,
                        uint32_t *dst, size_t cap, int *w, int *h);
/* What object (if any) draws at a pixel of the 360x243 frame: kind as above
 * (-1 none), index, topmost first, written into out[] (max entries). */
typedef struct { int kind, index; } arcdebug_hit;
int  arcdebug_vdc_hit_test(arcdebug *d, int x, int y, arcdebug_hit *out, int max);

/* The Videopac+ G7400's EF9340/41 text chip. present = 0 on the other
 * machines (a window shows "Videopac+ G7400 only"). */
int  arcdebug_ef_present(arcdebug *d);
void arcdebug_ef_get(arcdebug *d, ef9340_snapshot_t *out);
/* The text layer rendered alone (XRGB, W x H in *w,*h). */
int  arcdebug_ef_view(arcdebug *d, uint32_t *dst, size_t cap, int *w, int *h);

/* ---- sound and I/O ------------------------------------------------------------ */
typedef struct {
    int machine;
    int have_bios, have_font;            /* else the stand-in / a blank font */
    int stub_running;                    /* the stand-in is the BIOS now */
    int missing;                         /* ARC_MISSING_* */
    /* P1 / P2 decoded by function (arcadia2.cpp) */
    int p1, p2;
    int cart_bank;                       /* P10, P11 */
    int kbd_enable;                      /* P12 low: the decoder and stick buffer on */
    int vdc_select;                      /* P13 low */
    int ram_select;                      /* P14 low: external RAM and the cartridge */
    int ef_select;                       /* P15 low (G7400) */
    int copy_mode;                       /* P16 high: external writes and VDC reads gated off */
    int lum;                             /* P17 low: every pixel brightened */
    int kbd_row;                         /* P20-P22 */
    /* inputs as the machine has them */
    uint64_t keys;                       /* the matrix, bit row*8+col */
    uint8_t sticks[2];                   /* bit 0 UP, 1 RIGHT, 2 DOWN, 3 LEFT, 4 BUTTON */
    int reset_held;
    /* the 8244's sound is in the VDC snapshot; The Voice's: */
    int voice_present;
    sp0256_debug_t voice;
    int t0;                              /* the cart's T0 (The Voice's !LRQ) */
    uint64_t audio_samples;
} arcdebug_io;
void arcdebug_io_get(arcdebug *d, arcdebug_io *out);

/* ---- the cartridge ------------------------------------------------------------ */
typedef struct {
    int present;
    int type;                            /* enum arc_cart_type (carts.h) */
    const char *type_name;               /* "FujiNet", "Standard", "4-in-1", ... */
    int voice_in_front;
    /* a mapper cartridge */
    uint32_t rom_size;
    int bank, control;
    /* the FujiNet cartridge (type FujiNet) */
    int link_up, link_off, mailbox, app_live, booted_game, armed;
    int staged_app, staged_mailbox;
    uint32_t live_size, live_crc, staged_size, staged_crc;
    int swaps;
    int nbanks, fbank;                   /* legacy banks / APPBANK pages, and the one shown */
    int boot_state, boot_pct, boot_err;
    uint8_t ackseq, status, last_error, reply_cmd;
    uint16_t rxlen;
    uint32_t queue_depth;
    uint8_t mailbox_page[0xE0];          /* $F20-$FFF as the console reads it */
} arcdebug_cart;
void arcdebug_cart_get(arcdebug *d, arcdebug_cart *out);
/* One line: "FujiNet cartridge: CONFIG, link up". */
int  arcdebug_cart_info(arcdebug *d, char *dst, int dstsz);

/* ---- FujiNet: the cartridge's socket, as FujiBus packets ------------------------ */
typedef struct {
    uint64_t serial;
    int      to_host;       /* 1: FujiNet -> Arcadia 2001, 0: Arcadia 2001 -> FujiNet */
    int      device, command, length;   /* from the FujiBus header, -1 if short */
    int      nbytes;
    uint8_t  data[64];      /* the decoded packet's first bytes */
    char     text[96];      /* "Arcadia 2001 -> FujiNet 70 F4 READ HOST SLOTS (6 bytes)" */
    uint32_t repeat;        /* the same exchange back to back, shown "xN" */
} arcdebug_fuji_packet;
/* Recorded from session start, attached or not. The newest `max`, oldest
 * first; returns the count. */
int  arcdebug_fujinet_trace(arcdebug *d, arcdebug_fuji_packet *out, int max);
void arcdebug_fujinet_trace_clear(arcdebug *d);
/* The cartridge's link and mailbox, as text lines. Returns length. */
int  arcdebug_fujinet_state(arcdebug *d, char *dst, int dstsz);

/* ---- breakpoints -------------------------------------------------------------- */
typedef enum {
    ARCDEBUG_BP_EXEC = 1, ARCDEBUG_BP_READ = 2, ARCDEBUG_BP_WRITE = 4,
    ARCDEBUG_BP_IN = 8, ARCDEBUG_BP_OUT = 16
} arcdebug_bp_type;
typedef struct {
    int id;
    int type;                /* arcdebug_bp_type bits */
    int space;               /* arcdebug_space for READ/WRITE; for IN/OUT: 1 = P1, 2 = P2, 0 = BUS */
    uint16_t start, end;     /* an address range */
    int enabled;
    unsigned hits;
    char condition[128];     /* may use value, addr, registers, flags, vpos, hpos, the VDC's status */
} arcdebug_breakpoint;
/* Execute breakpoint at addr: added if absent, removed if present. Returns
 * 1 when one is now set. */
int  arcdebug_breakpoint_toggle(arcdebug *d, uint16_t addr);
int  arcdebug_breakpoint_check(arcdebug *d, uint16_t addr);
/* Returns the new id, or -1 if the condition does not parse. */
int  arcdebug_breakpoint_add(arcdebug *d, int type, int space, uint16_t start,
                            uint16_t end, const char *condition);
void arcdebug_breakpoint_remove(arcdebug *d, int id);
void arcdebug_breakpoint_enable(arcdebug *d, int id, int enabled);
int  arcdebug_breakpoint_list(arcdebug *d, arcdebug_breakpoint *out, int max);
void arcdebug_breakpoint_clear(arcdebug *d);

/* ---- the trace ring ----------------------------------------------------------- */
typedef struct {
    uint16_t pc;
    uint8_t a, psw, sp, r0, r1;
    uint8_t len, bytes[2];
    uint64_t cycles;
    int16_t vpos, hpos;
} arcdebug_trace;
void arcdebug_trace_enable(arcdebug *d, int on);
int  arcdebug_trace_enabled(arcdebug *d);
/* Newest first; returns entries copied. */
int  arcdebug_trace_read(arcdebug *d, arcdebug_trace *out, int max);
void arcdebug_trace_clear(arcdebug *d);

/* ---- files -------------------------------------------------------------------- */
/* Save with the path a native file picker supplied. kind: "dis" (the BIOS and
 * the cartridge as code, as mapped now), "ram" (the 64 internal and 128
 * external bytes), "mailbox" (the cart's $F20-$FFF page), "vdc" (the 256
 * register bytes), "font" (the character generator), "screen" (the frame as a
 * PPM), "regs" (CPU, VDC and I/O state as text). Message into msg; returns 0
 * or -1. */
int  arcdebug_save(arcdebug *d, const char *kind, const char *path,
                       char *msg, int msgsz);

#ifdef __cplusplus
}
#endif

#endif /* ARCDEBUG_H */
