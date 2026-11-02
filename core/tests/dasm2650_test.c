/*
 * dasm2650_test -- the 2650 disassembler against MAME's own (dasm2650_golden.h,
 * from 2650dasm.cpp compiled standalone by tools/ab/gen_dasm2650_golden.sh):
 * every opcode's text, length and STEP_* flags at 16 pc/operand triples; then
 * the branch targets and the disassembler against the CPU core: every legal
 * opcode advances the PC by the printed length, the illegal ones are the
 * ones the core counts as illegal.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dasm2650.h"
#include "s2650.h"
#include "dasm2650_golden.h"

static int failures;
static int checks;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

static uint8_t mem[0x8000];

static uint8_t mread(void *ctx, uint16_t a)
{
    (void)ctx;
    return mem[a & 0x7fff];
}

int main(void)
{
    char buf[64], what[240];
    uint32_t flags;
    int texts = 0;

    /* ---- every opcode against MAME ---- */
    for (int op = 0; op < 256; op++) {
        const int nvar = dasm2650_golden[op].len > 1 ? DASM2650_NV : 1;
        for (int vi = 0; vi < nvar; vi++) {
            const uint8_t bytes[3] = { (uint8_t)op, dasm2650_variants[vi].b1, dasm2650_variants[vi].b2 };
            const int len = dasm2650_flags(dasm2650_variants[vi].pc, bytes, buf, sizeof buf, &flags);

            snprintf(what, sizeof what, "%02X @%04X b1=%02X b2=%02X: \"%s\" (%d) vs MAME's \"%s\" (%d)", op,
                     dasm2650_variants[vi].pc, bytes[1], bytes[2], buf, len, dasm2650_golden[op].text[vi],
                     dasm2650_golden[op].len);
            check(len == dasm2650_golden[op].len && strcmp(buf, dasm2650_golden[op].text[vi]) == 0, what);
            texts++;

            snprintf(what, sizeof what, "%02X: flags %08X vs MAME's %X", op, (unsigned)flags,
                     dasm2650_golden[op].flags);
            check((flags >> 28) == dasm2650_golden[op].flags && (flags & 0x0fffffffu) == 0, what);
        }
        snprintf(what, sizeof what, "%02X: dasm2650_length %d vs %d", op, dasm2650_length((uint8_t)op),
                 dasm2650_golden[op].len);
        check(dasm2650_length((uint8_t)op) == dasm2650_golden[op].len, what);
        snprintf(what, sizeof what, "%02X: dasm2650_op_flags", op);
        check((dasm2650_op_flags((uint8_t)op) >> 28) == dasm2650_golden[op].flags, what);
    }

    /* ---- the 4-argument form and the buffer limit ---- */
    {
        const uint8_t bytes[3] = { 0x04, 0x5a, 0 };
        char tiny[6];
        memset(tiny, 'x', sizeof tiny);
        check(dasm2650(0, bytes, tiny, 5) == 2, "a short buffer still reports the length");
        check(tiny[4] == 0 && strncmp(tiny, "lodi", 4) == 0, "a short buffer is cut and terminated");
        check(dasm2650(0, bytes, NULL, 0) == 2, "a zero-size buffer is only measured");
        check(dasm2650_operand2_addr(0x1ffe) == 0x0000 && dasm2650_operand2_addr(0x0100) == 0x0102 &&
                  dasm2650_operand2_addr(0x1fff) == 0x2001,
              "dasm2650_operand2_addr wraps like MAME's");
    }

    /* ---- targets ---- */
    {
        const uint8_t bcta[3] = { 0x1f, 0x05, 0x37 };       /* bcta,a $0537 */
        const uint8_t bcta_i[3] = { 0x1f, 0x85, 0x37 };     /* indirect */
        const uint8_t bctr[3] = { 0x1b, 0x7e, 0 };          /* bctr,a pc+2-2 */
        const uint8_t bctr_i[3] = { 0x1b, 0xfe, 0 };
        const uint8_t zbsr[3] = { 0xbb, 0x22, 0 };
        const uint8_t bsxa[3] = { 0xbf, 0x01, 0x02 };
        const uint8_t nop[3] = { 0xc0, 0, 0 };
        check(dasm2650_target(0x123, bcta) == 0x537, "BCTA target");
        check(dasm2650_target(0x123, bcta_i) == -1, "indirect BCTA has no static target");
        check(dasm2650_target(0x123, bctr) == 0x123 + 2 - 2, "BCTR target (offset -2)");
        check(dasm2650_target(0x123, bctr_i) == -1, "indirect BCTR");
        check(dasm2650_target(0x123, zbsr) == 0x22, "ZBSR target");
        check(dasm2650_target(0x123, bsxa) == -1, "BSXA needs R3");
        check(dasm2650_target(0x123, nop) == -1, "no target for a NOP");
    }

    /* ---- the core against the disassembler ---- */
    {
        s2650_t cpu;
        int illegal_ops = 0;
        s2650_init(&cpu);
        cpu.read = mread;
        cpu.ctx = NULL;

        for (int op = 0; op < 256; op++) {
            const uint8_t bytes[3] = { (uint8_t)op, 0x00, 0x00 };
            const int len = dasm2650_flags(0x0100, bytes, buf, sizeof buf, &flags);
            const int is_illegal = strncmp(buf, "****", 4) == 0;
            unsigned before;

            s2650_reset(&cpu);
            memset(mem, 0, sizeof mem);
            mem[0x0100] = (uint8_t)op;
            s2650_set_pc(&cpu, 0x0100);
            before = cpu.illegal_count;
            s2650_step(&cpu);

            if (is_illegal) {
                illegal_ops++;
                snprintf(what, sizeof what, "%02X prints illegal and the core counts it", op);
                check(cpu.illegal_count == before + 1, what);
            } else {
                snprintf(what, sizeof what, "%02X \"%s\": the core does not count it illegal", op, buf);
                check(cpu.illegal_count == before, what);
            }
            /* straight-line: not a branch/return/halt */
            if (!is_illegal && op != 0x40 && !(flags & (DASM2650_STEP_OUT | DASM2650_STEP_OVER)) &&
                !strstr(buf, "bct") && !strstr(buf, "bcf") && !strstr(buf, "brn") && !strstr(buf, "bir") &&
                !strstr(buf, "bdr") && !strstr(buf, "zbrr") && !strstr(buf, "bxa")) {
                snprintf(what, sizeof what, "%02X \"%s\": PC advanced by %d, now %04X", op, buf, len,
                         s2650_pc(&cpu));
                check(s2650_pc(&cpu) == 0x0100 + len, what);
            }
        }
        snprintf(what, sizeof what, "%d illegal opcodes (expected 10)", illegal_ops);
        check(illegal_ops == 10, what);
    }

    printf("dasm2650_test: %d checks (%d texts), %d failures\n", checks, texts, failures);
    return failures ? 1 : 0;
}
