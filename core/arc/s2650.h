/* s2650.h -- the Signetics 2650, the Emerson Arcadia 2001's CPU.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * (transposed from MAME src/devices/cpu/s2650/s2650.cpp and s2650.h;
 * copyright-holders Juergen Buchmueller -- see COMPLIANCE.md)
 *
 * State and function names keep MAME's own, minus the m_ prefix, so the two
 * can be diffed side by side. One s2650_step() is one pass of MAME's
 * execute_run loop: take a pending interrupt, then fetch and execute one
 * instruction.
 *
 * Timing. `cycles` counts the CPU's own clock (MAME's m_icount unit: 6, 9 or
 * 12 clocks per instruction, so the 2650 at 3.58 MHz / 4 runs 0.895 MHz / 3
 * instruction-cycles) since reset. As in MAME the opcode's cycles are burnt
 * BEFORE it executes (conditional extras - a taken RETC, an indirect address -
 * when they are decided), so a callback made by an instruction sees `cycles`
 * already advanced past the instruction's base cost.
 *
 * Differences from MAME, all deliberate (see also s2650.c):
 *  - MAME checks the IRQ line only at the start of a timeslice and after a
 *    CPSU/RETE; s2650_step() checks it before every instruction (the real
 *    chip samples at each instruction end), so an interrupt is taken at the
 *    first boundary after the line rises while MAME's waits for the next
 *    slice. Machines that raise the line from a timer see no difference at
 *    instruction granularity beyond that latency.
 *  - A halted CPU re-executes HALT (PC stays on it, 6 cycles per step) where
 *    MAME burns the rest of its slice; the interrupt wakes it as in MAME.
 *  - The interrupt acknowledge (MAME's intack_handler, a read8 returning the
 *    vector byte, default 0) is the `intack` callback; the debugger's
 *    debugger_instruction_hook is `instruction_hook`.
 *  - Opcode/operand fetches may go through a separate `fetch` callback
 *    (MAME's cached program reader), so a debugger's read breakpoints do not
 *    trip on instruction fetches; NULL uses `read`.
 *  - Unwired callbacks read what an undriven bus does: memory and I/O 0xFF,
 *    sense 0, intack vector 0.
 */

#ifndef ARC_S2650_H
#define ARC_S2650_H

#include <stdbool.h>
#include <stdint.h>

#include "dasm2650.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PSL bits */
#define S2650_C     0x01    /* carry */
#define S2650_COM   0x02    /* compare: 0 signed, 1 unsigned (2's complement ops) */
#define S2650_OVF   0x04
#define S2650_WC    0x08    /* with carry */
#define S2650_RS    0x10    /* register select: 0 R1-R3, 1 R1'-R3' */
#define S2650_IDC   0x20    /* inter-digit carry */
#define S2650_CC    0xc0    /* condition code: 0x00 zero, 0x40 plus, 0x80 minus */
/* PSU bits */
#define S2650_SP    0x07    /* return stack pointer */
#define S2650_PSU34 0x18    /* unused */
#define S2650_II    0x20    /* interrupt inhibit */
#define S2650_FO    0x40    /* flag output */
#define S2650_SI    0x80    /* sense input */

/* s2650_set_irq() lines (INPUT_LINE_IRQ0 / S2650_SENSE_LINE) */
#define S2650_IRQ_LINE    0
#define S2650_SENSE_LINE  1

/* io_read / io_write ports: the non-extended D/~C pair, then extended 0-$FF */
#define S2650_CTRL_PORT   0x100     /* REDC / WRTC */
#define S2650_DATA_PORT   0x101     /* REDD / WRTD */

typedef struct s2650 s2650_t;

struct s2650 {
    /* ---- the machine. Every callback may be NULL (see above). ---- */
    void *ctx;
    /* memory space: 15-bit address */
    uint8_t (*read)(void *ctx, uint16_t a15);
    void (*write)(void *ctx, uint16_t a15, uint8_t data);
    /* opcode and operand fetches (optional; NULL = read) */
    uint8_t (*fetch)(void *ctx, uint16_t a15);
    /* I/O: S2650_CTRL_PORT / S2650_DATA_PORT, or the extended port 0-$FF */
    uint8_t (*io_read)(void *ctx, int port);
    void (*io_write)(void *ctx, int port, uint8_t data);
    /* the SENSE pin, polled whenever PSU is read (SPSU, TPSU); if NULL the
     * SI bit is whatever s2650_set_irq(.., S2650_SENSE_LINE, ..) left */
    int (*sense)(void *ctx);
    /* the FLAG pin: called when PSU's FO bit changes */
    void (*flag)(void *ctx, int level);
    /* interrupt acknowledge: returns the vector byte (bit 7 = indirect) */
    uint8_t (*intack)(void *ctx);
    /* the debugger's instruction hook: the address of the instruction about
     * to be fetched, after any interrupt entry. Cannot stop the step. */
    void (*instruction_hook)(void *ctx, uint16_t pc);

    /* ---- MAME's state ---- */
    uint16_t ppc;       /* previous program counter (page + iar) */
    uint16_t page;      /* 8K page select register (A14..A13) */
    uint16_t iar;       /* instruction address register (A12..A0) */
    uint16_t ea;        /* effective address (A14..A0) */
    uint8_t psl;        /* processor status lower */
    uint8_t psu;        /* processor status upper */
    uint8_t r;          /* absolute addressing dst/src register */
    uint8_t reg[7];     /* R0, R1-R3 of the active set, R1'-R3' (the inactive set) */
    uint8_t halt;       /* 1 if the cpu is halted */
    uint8_t ir;         /* instruction register */
    uint16_t ras[8];    /* the 8 return address stack entries */
    uint8_t irq_state;  /* the IRQ line */

    /* ---- not in MAME: the host's and the debugger's view ---- */
    uint64_t cycles;            /* clocks since reset (see above) */
    uint64_t instructions;      /* instructions executed (an interrupt entry
                                   counts with the instruction it precedes) */
    uint64_t start_cycles;      /* `cycles` when the current/last instruction began (a callback
                                   made by it can derive the beam position from this: MAME's
                                   timers advance between instructions) */
    int step_cycles;            /* what the last s2650_step() consumed */
    bool irq_taken;             /* the last s2650_step() entered an interrupt */
    unsigned illegal_count;     /* illegal opcodes executed (MAME logs them) */
    uint16_t illegal_pc;        /* the last one's address */
};

/* MAME's device_start: everything zero, callbacks left as the caller set
 * them (the struct is cleared first; set the callbacks after this). Does not
 * reset; call s2650_reset() next. */
void s2650_init(s2650_t *s);

/* MAME's device_reset: PC 0, registers and return stack zero, PSL = COM|WC,
 * PSU 0 (FO low: the flag callback sees the change from a forced 0xFF),
 * the IRQ line low, halt cleared. */
void s2650_reset(s2650_t *s);

/* One pass of MAME's execute_run loop: take an interrupt if one is pending
 * and not inhibited, then fetch and execute one instruction. Returns the
 * cycles consumed (an interrupt entry adds 9, 15 when the vector is
 * indirect). The IRQ line is sampled here. */
int s2650_step(s2650_t *s);

/* S2650_IRQ_LINE: the interrupt request (level, held until the machine
 * clears it); S2650_SENSE_LINE: the SENSE input (SI in PSU). */
void s2650_set_irq(s2650_t *s, int line, bool asserted);

/* ---- debugger helpers (pure unless noted) ---- */

static inline uint16_t s2650_pc(const s2650_t *s) { return (uint16_t)(s->page + s->iar); }
static inline void s2650_set_pc(s2650_t *s, uint16_t pc)
{
    s->page = pc & 0x6000;
    s->iar = pc & 0x1fff;
}
static inline int s2650_sp(const s2650_t *s) { return s->psu & S2650_SP; }
static inline uint8_t s2650_psu_raw(const s2650_t *s) { return s->psu; }
static inline uint8_t s2650_psl_raw(const s2650_t *s) { return s->psl; }

/* R0 / R1-R3 of register bank 0 or 1 (n = 0-3; bank 1 is R1'-R3'), whichever
 * set is active (PSL RS) being in reg[1-3]. */
static inline uint8_t s2650_reg_bank(const s2650_t *s, int bank, int n)
{
    n &= 3;
    if (n == 0)
        return s->reg[0];
    return s->reg[((s->psl & S2650_RS) ? 1 : 0) == (bank ? 1 : 0) ? n : n + 3];
}
static inline void s2650_set_reg_bank(s2650_t *s, int bank, int n, uint8_t v)
{
    n &= 3;
    s->reg[n == 0 ? 0 : (((s->psl & S2650_RS) ? 1 : 0) == (bank ? 1 : 0) ? n : n + 3)] = v;
}
/* The active set's R0-R3. */
static inline uint8_t s2650_reg(const s2650_t *s, int n) { return s->reg[n & 3]; }

/* The return stack entry n (0-7, the 15-bit address). */
static inline uint16_t s2650_stack(const s2650_t *s, int n) { return s->ras[n & 7]; }

/* Debugger writes (state_import): they have MAME's side effects. */
void s2650_set_psu(s2650_t *s, uint8_t psu);    /* FO change calls flag; II/SP as given */
void s2650_set_psl(s2650_t *s, uint8_t psl);    /* an RS change swaps R1-R3 / R1'-R3' */

/* PSU as an instruction reads it: polls the sense callback (side effect). */
uint8_t s2650_get_psu(s2650_t *s);

/* For step over / step out: the flags (DASM2650_STEP_*) of an opcode. */
static inline uint32_t s2650_op_flags(uint8_t opcode) { return dasm2650_op_flags(opcode); }
static inline bool s2650_op_is_call(uint8_t opcode) { return (dasm2650_op_flags(opcode) & DASM2650_STEP_OVER) != 0; }
static inline bool s2650_op_is_return(uint8_t opcode) { return (dasm2650_op_flags(opcode) & DASM2650_STEP_OUT) != 0; }

#ifdef __cplusplus
}
#endif

#endif /* ARC_S2650_H */
