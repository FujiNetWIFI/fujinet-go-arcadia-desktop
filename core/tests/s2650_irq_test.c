/*
 * s2650_irq_test -- what the A/B against MAME cannot reach: interrupts, HALT,
 * the FLAG and SENSE pins, and the core's own API. The Arcadia's MAME driver
 * never raises the 2650's IRQ and Lua cannot set an input line, and a halted
 * MAME core burns the rest of its timeslice, so these expectations come from
 * reading s2650.cpp (check_irq_line, M_RETE, M_CPSU, HALT, set_psu) and are
 * written out by hand.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "s2650.h"

static int failures;
static int checks;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        checks++;                                                                   \
        if (!(cond)) {                                                              \
            printf("FAIL line %d: %s\n", __LINE__, #cond);                          \
            failures++;                                                             \
        }                                                                           \
    } while (0)

typedef struct {
    uint8_t mem[0x8000];
    int vector;
    int flag_calls;
    int flag_last;
    int sense_value;
    int reads, fetches;
    int intack_calls;
} env_t;

static uint8_t rd(void *c, uint16_t a) { env_t *e = c; e->reads++; return e->mem[a & 0x7fff]; }
static uint8_t ft(void *c, uint16_t a) { env_t *e = c; e->fetches++; return e->mem[a & 0x7fff]; }
static void wr(void *c, uint16_t a, uint8_t d) { env_t *e = c; e->mem[a & 0x7fff] = d; }
static int sn(void *c) { env_t *e = c; return e->sense_value; }
static void fl(void *c, int v) { env_t *e = c; e->flag_calls++; e->flag_last = v; }
static uint8_t ia(void *c) { env_t *e = c; e->intack_calls++; return (uint8_t)e->vector; }

static void fresh(env_t *e, s2650_t *s)
{
    memset(e, 0, sizeof *e);
    memset(e->mem, 0xc0, sizeof e->mem);        /* NOPs */
    s2650_init(s);
    s->ctx = e;
    s->read = rd;
    s->write = wr;
    s->intack = ia;
    s2650_reset(s);
}

int main(void)
{
    env_t e;
    s2650_t s;

    /* ---- reset state ---- */
    fresh(&e, &s);
    CHECK(s.psl == (S2650_COM | S2650_WC));
    CHECK(s.psu == 0);
    CHECK(s2650_pc(&s) == 0);
    CHECK(s.halt == 0 && s.irq_state == 0);

    /* ---- IRQ: vector 0 is a ZBSR to $0000 (9 clocks), II set, return pushed ---- */
    fresh(&e, &s);
    e.mem[0x0000] = 0xc0;                       /* the handler: NOP */
    s2650_set_pc(&s, 0x2123);
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    int cyc = s2650_step(&s);
    CHECK(s.irq_taken);
    CHECK(cyc == 9 + 6);                        /* entry + the NOP */
    CHECK(s2650_sp(&s) == 1);
    CHECK(s.ras[1] == 0x2123);
    CHECK(s.psu & S2650_II);
    CHECK(s2650_pc(&s) == 0x0001);              /* NOP at $0000 executed */
    CHECK(e.intack_calls == 1);
    /* inhibited now: the line stays asserted but is not taken again */
    cyc = s2650_step(&s);
    CHECK(!s.irq_taken && cyc == 6 && e.intack_calls == 1);

    /* ---- RETE returns, clears II and re-enters at once if the line is still asserted ---- */
    fresh(&e, &s);
    e.mem[0x0000] = 0x37;                       /* RETE,3 */
    s2650_set_pc(&s, 0x0200);
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    s2650_step(&s);                             /* enters, runs RETE: back at $0200, II clear, line high -> again */
    CHECK(s2650_sp(&s) == 1);                   /* re-entered: pushed again */
    CHECK(s.ras[1] == 0x0200);
    CHECK(s.psu & S2650_II);
    CHECK(s2650_pc(&s) == 0x0000);
    CHECK(e.intack_calls == 2);
    s2650_set_irq(&s, S2650_IRQ_LINE, false);
    s2650_step(&s);                             /* RETE again: no re-entry */
    CHECK(s2650_sp(&s) == 0 && !(s.psu & S2650_II) && s2650_pc(&s) == 0x0200);

    /* ---- II inhibits; CPSU II with the line pending takes it (9 more clocks) ---- */
    fresh(&e, &s);
    e.mem[0x0100] = 0x76; e.mem[0x0101] = S2650_II;     /* PPSU II */
    e.mem[0x0102] = 0x74; e.mem[0x0103] = S2650_II;     /* CPSU II */
    s2650_set_pc(&s, 0x0100);
    s2650_step(&s);                             /* PPSU II */
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    cyc = s2650_step(&s);                       /* CPSU: 9 + interrupt entry 9, then... */
    CHECK(s.irq_taken);                         /* (the entry happens inside CPSU, after the step's own check) */
    CHECK(cyc == 9 + 9);
    CHECK(s2650_sp(&s) == 1 && s.ras[1] == 0x0104 && (s.psu & S2650_II));
    CHECK(s2650_pc(&s) == 0x0000);

    /* ---- indirect vector: bit 7 set -> the pointer at (sext7 & $1FFF) ---- */
    fresh(&e, &s);
    e.vector = 0x80 | 0x10;                     /* pointer at $0010 */
    e.mem[0x0010] = 0x12; e.mem[0x0011] = 0x34; /* -> $1234 */
    e.mem[0x1234] = 0xc0;
    s2650_set_pc(&s, 0x0050);
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    cyc = s2650_step(&s);
    CHECK(cyc == 9 + 6 + 6);
    CHECK(s.ras[1] == 0x0050 && s2650_pc(&s) == 0x1235);

    /* ---- a negative direct vector reaches the top of page 0 ---- */
    fresh(&e, &s);
    e.vector = 0x7f;                            /* sext7 = -1 -> $1FFF */
    s2650_set_pc(&s, 0x0050);
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    s2650_step(&s);
    CHECK(s.ras[1] == 0x0050);
    CHECK((s2650_pc(&s) & 0x6000) == 0 && (s2650_pc(&s) & 0x1fff) == 0x0000);   /* NOP at $1FFF, iar wraps in the page */

    /* ---- the interrupt vector's page: an interrupt taken in page 2 returns there and runs in page 0 ---- */
    fresh(&e, &s);
    s2650_set_pc(&s, 0x4010);
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    s2650_step(&s);
    CHECK(s.ras[1] == 0x4010 && (s2650_pc(&s) & 0x6000) == 0);

    /* ---- HALT: PC stays, 6 clocks a step; an interrupt wakes it and returns after the HALT ---- */
    fresh(&e, &s);
    e.mem[0x0100] = 0x40;
    s2650_set_pc(&s, 0x0100);
    cyc = s2650_step(&s);
    CHECK(cyc == 6 && s.halt == 1 && s2650_pc(&s) == 0x0100);
    cyc = s2650_step(&s);
    CHECK(cyc == 6 && s2650_pc(&s) == 0x0100);
    s2650_set_irq(&s, S2650_IRQ_LINE, true);
    s2650_step(&s);
    CHECK(s.halt == 0 && s.ras[1] == 0x0101);

    /* ---- the stack: eight deep, the ninth overwrites the first ---- */
    fresh(&e, &s);
    for (int i = 0; i < 9; i++) {
        e.mem[0x0100 + 3 * i] = 0x3f;           /* BSTA,3 next */
        e.mem[0x0101 + 3 * i] = 0x01;
        e.mem[0x0102 + 3 * i] = (uint8_t)(0x03 + 3 * i);
    }
    s2650_set_pc(&s, 0x0100);
    for (int i = 0; i < 9; i++)
        s2650_step(&s);
    CHECK(s2650_sp(&s) == 1);                   /* 9 pushes from 0 wrap to 1 */
    CHECK(s.ras[1] == 0x0100 + 3 * 8 + 3);      /* the ninth return address overwrote the first */

    /* ---- FLAG pin: PPSU/CPSU FO call the handler on change only; reset calls it low ---- */
    fresh(&e, &s);
    s.flag = fl;
    s2650_reset(&s);
    CHECK(e.flag_calls == 1 && e.flag_last == 0);   /* forced 0xFF -> 0: FO fell */
    e.mem[0x0100] = 0x76; e.mem[0x0101] = S2650_FO;
    e.mem[0x0102] = 0x76; e.mem[0x0103] = S2650_FO;
    e.mem[0x0104] = 0x74; e.mem[0x0105] = S2650_FO;
    s2650_set_pc(&s, 0x0100);
    s2650_step(&s);
    CHECK(e.flag_calls == 2 && e.flag_last == 1);
    s2650_step(&s);
    CHECK(e.flag_calls == 2);                   /* no change */
    s2650_step(&s);
    CHECK(e.flag_calls == 3 && e.flag_last == 0);

    /* ---- SENSE pin: polled by SPSU/TPSU; PPSU cannot set SI, unused bits 3-4 stay clear ---- */
    fresh(&e, &s);
    s.sense = sn;
    e.mem[0x0100] = 0x12;                       /* SPSU */
    e.mem[0x0101] = 0x76; e.mem[0x0102] = 0xff; /* PPSU $FF */
    e.mem[0x0103] = 0x12;
    s2650_set_pc(&s, 0x0100);
    e.sense_value = 1;
    s2650_step(&s);
    CHECK(s.reg[0] == 0x80);
    s2650_step(&s);
    CHECK((s.psu & 0x18) == 0 && (s.psu & S2650_II) && (s.psu & S2650_FO) && (s.psu & 7) == 7);
    e.sense_value = 0;
    s2650_step(&s);
    CHECK(s.reg[0] == (S2650_II | S2650_FO | 7));   /* SPSU: unused bits masked, SI follows the pin */
    /* without a sense callback the SENSE line sets SI */
    fresh(&e, &s);
    s2650_set_irq(&s, S2650_SENSE_LINE, true);
    CHECK(s.psu & S2650_SI);
    e.mem[0x0100] = 0x12;
    s2650_set_pc(&s, 0x0100);
    s2650_step(&s);
    CHECK(s.reg[0] == 0x80);
    s2650_set_irq(&s, S2650_SENSE_LINE, false);
    CHECK(!(s.psu & S2650_SI));

    /* ---- fetch vs read callbacks ---- */
    fresh(&e, &s);
    s.fetch = ft;
    e.mem[0x0100] = 0x0c;                       /* LODA,0 $0200 */
    e.mem[0x0101] = 0x02; e.mem[0x0102] = 0x00;
    e.mem[0x0200] = 0x5a;
    s2650_set_pc(&s, 0x0100);
    s2650_step(&s);
    CHECK(e.fetches == 3 && e.reads == 1 && s.reg[0] == 0x5a);

    /* ---- register banks through the API ---- */
    fresh(&e, &s);
    s.reg[1] = 0x11; s.reg[4] = 0x44;           /* R1 / R1' */
    CHECK(s2650_reg_bank(&s, 0, 1) == 0x11 && s2650_reg_bank(&s, 1, 1) == 0x44);
    s2650_set_psl(&s, s.psl | S2650_RS);        /* swaps */
    CHECK(s.reg[1] == 0x44 && s2650_reg_bank(&s, 0, 1) == 0x11 && s2650_reg_bank(&s, 1, 1) == 0x44);
    s2650_set_reg_bank(&s, 0, 1, 0x99);
    CHECK(s.reg[4] == 0x99);
    s2650_set_psl(&s, s.psl & ~S2650_RS);
    CHECK(s.reg[1] == 0x99);

    /* ---- step-over / step-out hints ---- */
    CHECK(s2650_op_is_call(0x3f) && s2650_op_is_call(0xbb) && s2650_op_is_call(0xbf) && !s2650_op_is_call(0x1f));
    CHECK(s2650_op_is_return(0x17) && s2650_op_is_return(0x37) && !s2650_op_is_return(0x3f));

    /* ---- cycle counts: a taken RETC costs 6 more ---- */
    fresh(&e, &s);
    e.mem[0x0100] = 0x3f; e.mem[0x0101] = 0x02; e.mem[0x0102] = 0x00;   /* BSTA,3 $0200 */
    e.mem[0x0200] = 0x17;                                                 /* RETC,3 */
    e.mem[0x0201] = 0x14;                                                 /* RETC,0 */
    s2650_set_pc(&s, 0x0100);
    CHECK(s2650_step(&s) == 9);
    CHECK(s2650_step(&s) == 9 + 6);
    CHECK(s2650_pc(&s) == 0x0103);

    printf("s2650_irq_test: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
