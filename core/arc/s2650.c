/* s2650.c -- the Signetics 2650 (see s2650.h).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * (transposed from MAME src/devices/cpu/s2650/s2650.cpp; copyright-holders
 * Juergen Buchmueller -- see COMPLIANCE.md)
 *
 * The inline helpers and macros keep MAME's names and bodies (set_psu,
 * get_psu, check_irq_line, ROP, ARG, _REL_EA, _REL_ZERO, _ABS_EA, _BRA_EA,
 * SWAP_REGS, M_BRR ... M_TMI), with the m_ prefix dropped from the state
 * (it lives in `s`). s2650_step is one pass of execute_run, s2650_init /
 * s2650_reset are device_start / device_reset. The opcode switch is MAME's,
 * case for case, burning the same cycles at the same points.
 */

#include <string.h>

#include "s2650.h"

#define PMSK    0x1fff          /* mask page offset */
#define PLEN    0x2000          /* page length */
#define PAGE    0x6000          /* mask page */
#define AMSK    0x7fff          /* mask address range */

#define C       S2650_C
#define COM     S2650_COM
#define OVF     S2650_OVF
#define WC      S2650_WC
#define RS      S2650_RS
#define IDC     S2650_IDC
#define CC      S2650_CC
#define SP      S2650_SP
#define PSU34   S2650_PSU34
#define II      S2650_II
#define FO      S2650_FO
#define SI      S2650_SI

#define R0      (s->reg[0])

/* condition code changes for a byte */
static uint8_t ccc(unsigned v)
{
    v &= 0xff;
    return v == 0 ? 0x00 : (v < 0x80 ? 0x40 : 0x80);
}

/* util::sext(v, 7): the low 7 bits as a signed number */
static int sext7(int v)
{
    v &= 0x7f;
    return v & 0x40 ? v - 0x80 : v;
}

/* MAME burns an instruction's cycles (m_icount -= n) before it runs */
static inline void burn(s2650_t *s, int n)
{
    s->cycles += (unsigned)n;
    s->step_cycles += n;
}

static inline uint8_t RDMEM(s2650_t *s, unsigned addr)
{
    return s->read ? s->read(s->ctx, (uint16_t)(addr & AMSK)) : 0xff;
}
static inline void WRMEM(s2650_t *s, unsigned addr, uint8_t data)
{
    if (s->write)
        s->write(s->ctx, (uint16_t)(addr & AMSK), data);
}
static inline uint8_t cprogram_r(s2650_t *s, unsigned addr)
{
    if (s->fetch)
        return s->fetch(s->ctx, (uint16_t)(addr & AMSK));
    return RDMEM(s, addr);
}

static inline uint8_t data_r(s2650_t *s, int port)
{
    return s->io_read ? s->io_read(s->ctx, port) : 0xff;
}
static inline void data_w(s2650_t *s, int port, uint8_t v)
{
    if (s->io_write)
        s->io_write(s->ctx, port, v);
}

static inline void set_psu(s2650_t *s, uint8_t new_val)
{
    uint8_t old = s->psu;
    s->psu = new_val;
    if ((new_val ^ old) & FO) {
        if (s->flag)
            s->flag(s->ctx, (new_val & FO) ? 1 : 0);
    }
}

static inline uint8_t get_psu(s2650_t *s)
{
    if (s->sense) {
        if (s->sense(s->ctx))
            s->psu |= SI;
        else
            s->psu &= (uint8_t)~SI;
    }
    return s->psu;
}

static inline uint8_t get_sp(const s2650_t *s)
{
    return s->psu & SP;
}

static inline void set_sp(s2650_t *s, uint8_t new_sp)
{
    s->psu = (uint8_t)((s->psu & ~SP) | (new_sp & SP));
}

/* RDMEM of a big-endian pointer, wrapping inside its 8K page */
static inline unsigned indirect(s2650_t *s, unsigned addr)
{
    unsigned ea = (unsigned)RDMEM(s, addr) << 8;
    if ((++addr & PMSK) == 0)
        addr -= PLEN;                               /* page wrap */
    return (ea + RDMEM(s, addr)) & AMSK;
}

static inline int check_irq_line(s2650_t *s)
{
    int cycles = 0;
    if (s->irq_state) {
        if ((s->psu & II) == 0) {
            if (s->halt) {
                s->halt = 0;
                s->iar = (s->iar + 1) & PMSK;
            }
            /* standard_irq_callback(0, page + iar) is the host's business */
            /* Say hi */
            int vector = s->intack ? s->intack(s->ctx) : 0;
            /* build effective address within first 8K page */
            cycles += 9;    /* ZBSR */
            s->ea = (unsigned)sext7(vector) & PMSK;
            if (vector & 0x80) {    /* indirect bit set ? */
                cycles += 6;
                /* build indirect 32K address */
                s->ea = (uint16_t)indirect(s, s->ea);
            }
            set_sp(s, (uint8_t)(get_sp(s) + 1));
            set_psu(s, s->psu | II);
            s->ras[get_sp(s)] = (uint16_t)(s->page + s->iar);
            s->page = s->ea & PAGE;
            s->iar = s->ea & PMSK;
            s->irq_taken = true;
        }
    }
    return cycles;
}

#define SET_CC(result)  (s->psl = (uint8_t)((s->psl & ~CC) | ccc(result)))

static inline uint8_t ROP(s2650_t *s)
{
    uint8_t result = cprogram_r(s, (unsigned)(s->page + s->iar));
    s->iar = (s->iar + 1) & PMSK;
    return result;
}

static inline uint8_t ARG(s2650_t *s)
{
    uint8_t result = cprogram_r(s, (unsigned)(s->page + s->iar));
    s->iar = (s->iar + 1) & PMSK;
    return result;
}

/* _REL_EA: build effective address with relative addressing */
static inline void REL_EA(s2650_t *s, unsigned page)
{
    uint8_t hr = ARG(s);    /* get 'holding register' */
    /* build effective address within current 8K page */
    s->ea = (uint16_t)(page + ((s->iar + (unsigned)sext7(hr)) & PMSK));
    if (hr & 0x80) {        /* indirect bit set ? */
        burn(s, 6);
        s->ea = (uint16_t)indirect(s, s->ea);
    }
}

/* _REL_ZERO: build effective address with zero relative addressing */
static inline void REL_ZERO(s2650_t *s)
{
    uint8_t hr = ARG(s);    /* get 'holding register' */
    /* build effective address from 0 */
    s->ea = (uint16_t)((unsigned)sext7(hr) & PMSK);
    if (hr & 0x80) {        /* indirect bit set ? */
        burn(s, 6);
        s->ea = (uint16_t)indirect(s, s->ea);
    }
}

/* _ABS_EA: build effective address with absolute addressing */
static inline void ABS_EA(s2650_t *s)
{
    uint8_t hr, dr;
    hr = ARG(s);            /* get 'holding register' */
    dr = ARG(s);            /* get 'data bus register' */
    /* build effective address within current 8K page */
    s->ea = (uint16_t)(s->page + ((((unsigned)hr << 8) + dr) & PMSK));
    /* indirect addressing ? */
    if (hr & 0x80) {
        burn(s, 6);
        s->ea = (uint16_t)indirect(s, s->ea);
    }
    /* check indexed addressing modes */
    switch (hr & 0x60) {
    case 0x00: /* not indexed */
        break;
    case 0x20: /* auto increment indexed */
        s->reg[s->r] += 1;
        s->ea = (uint16_t)((s->ea & PAGE) + ((s->ea + s->reg[s->r]) & PMSK));
        s->r = 0; /* absolute addressing reg is R0 */
        break;
    case 0x40: /* auto decrement indexed */
        s->reg[s->r] -= 1;
        s->ea = (uint16_t)((s->ea & PAGE) + ((s->ea + s->reg[s->r]) & PMSK));
        s->r = 0; /* absolute addressing reg is R0 */
        break;
    case 0x60: /* indexed */
        s->ea = (uint16_t)((s->ea & PAGE) + ((s->ea + s->reg[s->r]) & PMSK));
        s->r = 0; /* absolute addressing reg is R0 */
        break;
    }
}

/* _BRA_EA: build effective address with absolute addressing (branch) */
static inline void BRA_EA(s2650_t *s)
{
    uint8_t hr, dr;
    hr = ARG(s);            /* get 'holding register' */
    dr = ARG(s);            /* get 'data bus register' */
    /* build address in 32K address space */
    s->ea = (uint16_t)((((unsigned)hr << 8) + dr) & AMSK);
    /* indirect addressing ? */
    if (hr & 0x80) {
        burn(s, 6);
        s->ea = (uint16_t)indirect(s, s->ea);
    }
}

/* SWAP_REGS: swap registers r1-r3 with r4-r6 (the second set); done every
 * time the RS bit in PSL changes */
static inline void SWAP_REGS(s2650_t *s)
{
    uint8_t tmp;
    tmp = s->reg[1]; s->reg[1] = s->reg[4]; s->reg[4] = tmp;
    tmp = s->reg[2]; s->reg[2] = s->reg[5]; s->reg[5] = tmp;
    tmp = s->reg[3]; s->reg[3] = s->reg[6]; s->reg[6] = tmp;
}

static inline void jump_ea(s2650_t *s)
{
    s->page = s->ea & PAGE;
    s->iar = s->ea & PMSK;
}

static inline void push_iar(s2650_t *s)
{
    set_sp(s, (uint8_t)(get_sp(s) + 1));
    s->ras[get_sp(s)] = (uint16_t)(s->page + s->iar);
}

/* M_BRR: branch relative if cond is true */
static inline void M_BRR(s2650_t *s, int cond)
{
    if (cond) {
        REL_EA(s, s->page);
        jump_ea(s);
    } else {
        s->iar = (s->iar + 1) & PMSK;
    }
}

/* M_ZBRR: branch relative to page zero */
static inline void M_ZBRR(s2650_t *s)
{
    REL_ZERO(s);
    jump_ea(s);
}

/* M_BRA: branch absolute if cond is true */
static inline void M_BRA(s2650_t *s, int cond)
{
    if (cond) {
        BRA_EA(s);
        jump_ea(s);
    } else {
        s->iar = (s->iar + 2) & PMSK;
    }
}

/* M_BXA: branch indexed absolute (EA + R3) */
static inline void M_BXA(s2650_t *s)
{
    BRA_EA(s);
    s->ea = (s->ea + s->reg[3]) & AMSK;
    jump_ea(s);
}

/* M_BSR: branch to subroutine relative if cond is true */
static inline void M_BSR(s2650_t *s, int cond)
{
    if (cond) {
        REL_EA(s, s->page);
        push_iar(s);
        jump_ea(s);
    } else {
        s->iar = (s->iar + 1) & PMSK;
    }
}

/* M_ZBSR: branch to subroutine relative to page zero */
static inline void M_ZBSR(s2650_t *s)
{
    REL_ZERO(s);
    push_iar(s);
    jump_ea(s);
}

/* M_BSA: branch to subroutine absolute */
static inline void M_BSA(s2650_t *s, int cond)
{
    if (cond) {
        BRA_EA(s);
        push_iar(s);
        jump_ea(s);
    } else {
        s->iar = (s->iar + 2) & PMSK;
    }
}

/* M_BSXA: branch to subroutine indexed absolute (EA + R3) */
static inline void M_BSXA(s2650_t *s)
{
    BRA_EA(s);
    s->ea = (s->ea + s->reg[3]) & AMSK;
    push_iar(s);
    jump_ea(s);
}

/* M_RET: return from subroutine if cond is true */
static inline void M_RET(s2650_t *s, int cond)
{
    if (cond) {
        burn(s, 6);
        s->ea = s->ras[get_sp(s)];
        set_sp(s, (uint8_t)(get_sp(s) - 1));
        jump_ea(s);
    }
}

/* M_RETE: return from subroutine if cond is true and enable interrupts;
 * afterwards check the IRQ line and eventually take the next interrupt */
static inline void M_RETE(s2650_t *s, int cond)
{
    if (cond) {
        s->ea = s->ras[get_sp(s)];
        set_sp(s, (uint8_t)(get_sp(s) - 1));
        jump_ea(s);
        set_psu(s, s->psu & ~II);
        burn(s, check_irq_line(s));
    }
}

/* M_LOD: load destination with source register */
static inline void M_LOD(s2650_t *s, uint8_t *dest, uint8_t source)
{
    *dest = source;
    SET_CC(*dest);
}

/* M_AND / M_IOR / M_EOR */
static inline void M_AND(s2650_t *s, uint8_t *dest, uint8_t source)
{
    *dest &= source;
    SET_CC(*dest);
}
static inline void M_IOR(s2650_t *s, uint8_t *dest, uint8_t source)
{
    *dest |= source;
    SET_CC(*dest);
}
static inline void M_EOR(s2650_t *s, uint8_t *dest, uint8_t source)
{
    *dest ^= source;
    SET_CC(*dest);
}

/* M_ADD: add source to destination; with carry if WC of PSL is set */
static inline void M_ADD(s2650_t *s, uint8_t *dest, uint8_t source)
{
    uint8_t before = *dest;
    /* add source; carry only if WC is set */
    uint16_t res = (uint16_t)(*dest + source + ((s->psl >> 3) & s->psl & C));
    s->psl &= (uint8_t)~(C | OVF | IDC);
    if (res & 0x100) s->psl |= C;
    *dest = res & 0xff;
    if ((*dest ^ before ^ source) & 0x10) s->psl |= IDC;
    if ((before ^ *dest) & (source ^ *dest) & 0x80) s->psl |= OVF;
    SET_CC(*dest);
}

/* M_SUB: subtract source from destination; with borrow if WC of PSL is set */
static inline void M_SUB(s2650_t *s, uint8_t *dest, uint8_t source)
{
    uint8_t before = *dest;
    /* subtract source; borrow only if WC is set */
    uint16_t res = (uint16_t)(*dest - source - ((s->psl >> 3) & (s->psl ^ C) & C));
    s->psl &= (uint8_t)~(C | OVF | IDC);
    if ((res & 0x100) == 0) s->psl |= C;
    *dest = res & 0xff;
    if (~(*dest ^ before ^ source) & 0x10) s->psl |= IDC;
    if ((before ^ source) & (before ^ *dest) & 0x80) s->psl |= OVF;
    SET_CC(*dest);
}

/* M_COM: compare register against value. If COM of PSL is set, use
 * unsigned, else signed comparison */
static inline void M_COM(s2650_t *s, uint8_t reg, uint8_t val)
{
    int16_t res;
    s->psl &= (uint8_t)~CC;
    if (s->psl & COM) res = (int16_t)((uint8_t)reg - (uint8_t)val);
    else res = (int16_t)((int8_t)reg - (int8_t)val);
    if (res < 0) s->psl |= 0x80;
    else if (res > 0) s->psl |= 0x40;
}

/* M_DAR: decimal adjust register */
static inline void M_DAR(s2650_t *s, uint8_t *dest)
{
    if ((s->psl & C) == 0) *dest = (uint8_t)(*dest + 0xa0);
    if ((s->psl & IDC) == 0) *dest = (uint8_t)((*dest & 0xf0) | ((*dest + 0x0a) & 0x0f));
}

/* M_RRL: rotate register left; if WC of PSL is set, rotate through carry,
 * else rotate circular */
static inline void M_RRL(s2650_t *s, uint8_t *dest)
{
    uint8_t before = *dest;
    if (s->psl & WC) {
        uint8_t c = s->psl & C;
        s->psl &= (uint8_t)~(C + IDC);
        *dest = (uint8_t)((before << 1) | c);
        s->psl |= (uint8_t)((before >> 7) + (*dest & IDC));
    } else {
        *dest = (uint8_t)((before << 1) | (before >> 7));
    }
    SET_CC(*dest);
    s->psl = (uint8_t)((s->psl & ~OVF) | (((*dest ^ before) >> 5) & OVF));
}

/* M_RRR: rotate register right; if WC of PSL is set, rotate through carry,
 * else rotate circular */
static inline void M_RRR(s2650_t *s, uint8_t *dest)
{
    uint8_t before = *dest;
    if (s->psl & WC) {
        uint8_t c = s->psl & C;
        s->psl &= (uint8_t)~(C + IDC);
        *dest = (uint8_t)((before >> 1) | (c << 7));
        s->psl |= (uint8_t)((before & C) + (*dest & IDC));
    } else {
        *dest = (uint8_t)((before >> 1) | (before << 7));
    }
    SET_CC(*dest);
    s->psl = (uint8_t)((s->psl & ~OVF) | (((*dest ^ before) >> 5) & OVF));
}

/* M_SPSU: store processor status upper (PSU) to register R0
 * (checks for the External Sense IO port) */
static inline void M_SPSU(s2650_t *s)
{
    R0 = get_psu(s) & ~PSU34;
    SET_CC(R0);
}

/* M_SPSL */
static inline void M_SPSL(s2650_t *s)
{
    R0 = s->psl;
    SET_CC(R0);
}

/* M_CPSU: clear processor status upper (PSU), selective */
static inline void M_CPSU(s2650_t *s)
{
    uint8_t cpsu = ARG(s) & ~SI;
    set_psu(s, s->psu & ~cpsu);
    burn(s, check_irq_line(s));
}

/* M_CPSL: clear processor status lower (PSL), selective */
static inline void M_CPSL(s2650_t *s)
{
    uint8_t cpsl = ARG(s);
    /* select other register set now ? */
    if ((cpsl & RS) && (s->psl & RS))
        SWAP_REGS(s);
    s->psl = s->psl & ~cpsl;
}

/* M_PPSU: preset processor status upper (PSU), selective; the unused bits
 * 3 and 4 can't be set */
static inline void M_PPSU(s2650_t *s)
{
    uint8_t ppsu = (ARG(s) & ~PSU34) & ~SI;
    set_psu(s, s->psu | ppsu);
}

/* M_PPSL: preset processor status lower (PSL), selective */
static inline void M_PPSL(s2650_t *s)
{
    uint8_t ppsl = ARG(s);
    /* select 2nd register set now ? */
    if ((ppsl & RS) && !(s->psl & RS))
        SWAP_REGS(s);
    s->psl = s->psl | ppsl;
}

/* M_TPSU: test processor status upper (PSU) */
static inline void M_TPSU(s2650_t *s)
{
    uint8_t tpsu = ARG(s);
    uint8_t rpsu = get_psu(s);
    s->psl &= (uint8_t)~CC;
    if ((rpsu & tpsu) != tpsu)
        s->psl |= 0x80;
}

/* M_TPSL: test processor status lower (PSL) */
static inline void M_TPSL(s2650_t *s)
{
    uint8_t tpsl = ARG(s);
    if ((s->psl & tpsl) != tpsl)
        s->psl = (uint8_t)((s->psl & ~CC) | 0x80);
    else
        s->psl &= (uint8_t)~CC;
}

/* M_TMI: test under mask immediate */
static inline void M_TMI(s2650_t *s, uint8_t value)
{
    uint8_t tmi = ARG(s);
    s->psl &= (uint8_t)~CC;
    if ((value & tmi) != tmi)
        s->psl |= 0x80;
}

/* ------------------------------------------------------------------------ */

void s2650_init(s2650_t *s)
{
    memset(s, 0, sizeof *s);
}

void s2650_reset(s2650_t *s)
{
    s->ppc = 0;
    s->page = 0;
    s->iar = 0;
    s->ea = 0;
    s->r = 0;
    s->halt = 0;
    s->ir = 0;
    s->irq_state = 0;
    memset(s->reg, 0, sizeof s->reg);
    memset(s->ras, 0, sizeof s->ras);

    s->psl = COM | WC;
    /* force write */
    s->psu = 0xff;
    set_psu(s, 0);

    s->step_cycles = 0;
    s->irq_taken = false;
}

void s2650_set_irq(s2650_t *s, int line, bool asserted)
{
    switch (line) {
    case S2650_IRQ_LINE:
        s->irq_state = asserted ? 1 : 0;
        break;
    case S2650_SENSE_LINE:  /* s2650_set_sense */
        if (asserted)
            set_psu(s, s->psu | SI);
        else
            set_psu(s, s->psu & ~SI);
        break;
    }
}

void s2650_set_psu(s2650_t *s, uint8_t psu)
{
    set_psu(s, psu);
}

void s2650_set_psl(s2650_t *s, uint8_t psl)
{
    if ((s->psl ^ psl) & RS)
        SWAP_REGS(s);
    s->psl = psl;
}

uint8_t s2650_get_psu(s2650_t *s)
{
    return get_psu(s);
}

int s2650_step(s2650_t *s)
{
    s->step_cycles = 0;
    s->irq_taken = false;
    s->start_cycles = s->cycles;

    /* check for external irqs (MAME: once per timeslice and after CPSU/RETE) */
    burn(s, check_irq_line(s));

    s->ppc = (uint16_t)(s->page + s->iar);

    if (s->instruction_hook)
        s->instruction_hook(s->ctx, (uint16_t)(s->page + s->iar));

    s->ir = ROP(s);
    s->r = s->ir & 3;       /* register / value */
    s->instructions++;
    switch (s->ir) {
    case 0x00: case 0x01: case 0x02: case 0x03:     /* LODZ,n */
        burn(s, 6);
        M_LOD(s, &R0, s->reg[s->r]);
        break;
    case 0x04: case 0x05: case 0x06: case 0x07:     /* LODI,n v */
        burn(s, 6);
        M_LOD(s, &s->reg[s->r], ARG(s));
        break;
    case 0x08: case 0x09: case 0x0a: case 0x0b:     /* LODR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_LOD(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x0c: case 0x0d: case 0x0e: case 0x0f:     /* LODA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_LOD(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x12:      /* SPSU */
        burn(s, 6);
        M_SPSU(s);
        break;
    case 0x13:      /* SPSL */
        burn(s, 6);
        M_SPSL(s);
        break;
    case 0x14: case 0x15: case 0x16:                /* RETC,n */
        burn(s, 9);     /* +6 cycles if condition is true */
        M_RET(s, (s->psl >> 6) == s->r);
        break;
    case 0x17:
        burn(s, 9);
        M_RET(s, 1);
        break;
    case 0x18: case 0x19: case 0x1a:                /* BCTR,n (*)a */
        burn(s, 9);
        M_BRR(s, (s->psl >> 6) == s->r);
        break;
    case 0x1b:
        burn(s, 9);
        M_BRR(s, 1);
        break;
    case 0x1c: case 0x1d: case 0x1e:                /* BCTA,n (*)a */
        burn(s, 9);
        M_BRA(s, (s->psl >> 6) == s->r);
        break;
    case 0x1f:
        burn(s, 9);
        M_BRA(s, 1);
        break;
    case 0x20: case 0x21: case 0x22: case 0x23:     /* EORZ,n */
        burn(s, 6);
        M_EOR(s, &R0, s->reg[s->r]);
        break;
    case 0x24: case 0x25: case 0x26: case 0x27:     /* EORI,n v */
        burn(s, 6);
        M_EOR(s, &s->reg[s->r], ARG(s));
        break;
    case 0x28: case 0x29: case 0x2a: case 0x2b:     /* EORR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_EOR(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x2c: case 0x2d: case 0x2e: case 0x2f:     /* EORA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_EOR(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x30: case 0x31: case 0x32: case 0x33:     /* REDC,n */
        burn(s, 6);
        s->reg[s->r] = data_r(s, S2650_CTRL_PORT);
        SET_CC(s->reg[s->r]);
        break;
    case 0x34: case 0x35: case 0x36:                /* RETE,n */
        burn(s, 9);
        M_RETE(s, (s->psl >> 6) == s->r);
        break;
    case 0x37:
        burn(s, 9);
        M_RETE(s, 1);
        break;
    case 0x38: case 0x39: case 0x3a:                /* BSTR,n (*)a */
        burn(s, 9);
        M_BSR(s, (s->psl >> 6) == s->r);
        break;
    case 0x3b:
        burn(s, 9);
        M_BSR(s, 1);
        break;
    case 0x3c: case 0x3d: case 0x3e:                /* BSTA,n (*)a */
        burn(s, 9);
        M_BSA(s, (s->psl >> 6) == s->r);
        break;
    case 0x3f:
        burn(s, 9);
        M_BSA(s, 1);
        break;
    case 0x40:      /* HALT */
        burn(s, 6);
        s->iar = (s->iar - 1) & PMSK;
        s->halt = 1;
        break;
    case 0x41: case 0x42: case 0x43:                /* ANDZ,n */
        burn(s, 6);
        M_AND(s, &R0, s->reg[s->r]);
        break;
    case 0x44: case 0x45: case 0x46: case 0x47:     /* ANDI,n v */
        burn(s, 6);
        M_AND(s, &s->reg[s->r], ARG(s));
        break;
    case 0x48: case 0x49: case 0x4a: case 0x4b:     /* ANDR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_AND(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x4c: case 0x4d: case 0x4e: case 0x4f:     /* ANDA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_AND(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x50: case 0x51: case 0x52: case 0x53:     /* RRR,n */
        burn(s, 6);
        M_RRR(s, &s->reg[s->r]);
        break;
    case 0x54: case 0x55: case 0x56: case 0x57:     /* REDE,n v */
        burn(s, 9);
        s->reg[s->r] = data_r(s, ARG(s));
        SET_CC(s->reg[s->r]);
        break;
    case 0x58: case 0x59: case 0x5a: case 0x5b:     /* BRNR,n (*)a */
        burn(s, 9);
        M_BRR(s, s->reg[s->r]);
        break;
    case 0x5c: case 0x5d: case 0x5e: case 0x5f:     /* BRNA,n (*)a */
        burn(s, 9);
        M_BRA(s, s->reg[s->r]);
        break;
    case 0x60: case 0x61: case 0x62: case 0x63:     /* IORZ,n */
        burn(s, 6);
        M_IOR(s, &R0, s->reg[s->r]);
        break;
    case 0x64: case 0x65: case 0x66: case 0x67:     /* IORI,n v */
        burn(s, 6);
        M_IOR(s, &s->reg[s->r], ARG(s));
        break;
    case 0x68: case 0x69: case 0x6a: case 0x6b:     /* IORR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_IOR(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x6c: case 0x6d: case 0x6e: case 0x6f:     /* IORA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_IOR(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x70: case 0x71: case 0x72: case 0x73:     /* REDD,n */
        burn(s, 6);
        s->reg[s->r] = data_r(s, S2650_DATA_PORT);
        SET_CC(s->reg[s->r]);
        break;
    case 0x74:      /* CPSU */
        burn(s, 9);
        M_CPSU(s);
        break;
    case 0x75:      /* CPSL */
        burn(s, 9);
        M_CPSL(s);
        break;
    case 0x76:      /* PPSU */
        burn(s, 9);
        M_PPSU(s);
        break;
    case 0x77:      /* PPSL */
        burn(s, 9);
        M_PPSL(s);
        break;
    case 0x78: case 0x79: case 0x7a: case 0x7b:     /* BSNR,n (*)a */
        burn(s, 9);
        M_BSR(s, s->reg[s->r]);
        break;
    case 0x7c: case 0x7d: case 0x7e: case 0x7f:     /* BSNA,n (*)a */
        burn(s, 9);
        M_BSA(s, s->reg[s->r]);
        break;
    case 0x80: case 0x81: case 0x82: case 0x83:     /* ADDZ,n */
        burn(s, 6);
        M_ADD(s, &R0, s->reg[s->r]);
        break;
    case 0x84: case 0x85: case 0x86: case 0x87:     /* ADDI,n v */
        burn(s, 6);
        M_ADD(s, &s->reg[s->r], ARG(s));
        break;
    case 0x88: case 0x89: case 0x8a: case 0x8b:     /* ADDR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_ADD(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x8c: case 0x8d: case 0x8e: case 0x8f:     /* ADDA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_ADD(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0x92:      /* LPSU */
        burn(s, 6);
        set_psu(s, (uint8_t)((R0 & ~PSU34 & ~SI) | (s->psu & SI)));
        break;
    case 0x93:      /* LPSL */
        burn(s, 6);
        /* change register set ? */
        if ((s->psl ^ R0) & RS)
            SWAP_REGS(s);
        s->psl = R0;
        break;
    case 0x94: case 0x95: case 0x96: case 0x97:     /* DAR,n */
        burn(s, 9);
        M_DAR(s, &s->reg[s->r]);
        break;
    case 0x98: case 0x99: case 0x9a:                /* BCFR,n (*)a */
        burn(s, 9);
        M_BRR(s, (s->psl >> 6) != s->r);
        break;
    case 0x9b:      /* ZBRR (*)a */
        burn(s, 9);
        M_ZBRR(s);
        break;
    case 0x9c: case 0x9d: case 0x9e:                /* BCFA,n (*)a */
        burn(s, 9);
        M_BRA(s, (s->psl >> 6) != s->r);
        break;
    case 0x9f:      /* BXA (*)a */
        burn(s, 9);
        M_BXA(s);
        break;
    case 0xa0: case 0xa1: case 0xa2: case 0xa3:     /* SUBZ,n */
        burn(s, 6);
        M_SUB(s, &R0, s->reg[s->r]);
        break;
    case 0xa4: case 0xa5: case 0xa6: case 0xa7:     /* SUBI,n v */
        burn(s, 6);
        M_SUB(s, &s->reg[s->r], ARG(s));
        break;
    case 0xa8: case 0xa9: case 0xaa: case 0xab:     /* SUBR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_SUB(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0xac: case 0xad: case 0xae: case 0xaf:     /* SUBA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_SUB(s, &s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0xb0: case 0xb1: case 0xb2: case 0xb3:     /* WRTC,n */
        burn(s, 6);
        data_w(s, S2650_CTRL_PORT, s->reg[s->r]);
        break;
    case 0xb4:      /* TPSU */
        burn(s, 9);
        M_TPSU(s);
        break;
    case 0xb5:      /* TPSL */
        burn(s, 9);
        M_TPSL(s);
        break;
    case 0xb8: case 0xb9: case 0xba:                /* BSFR,n (*)a */
        burn(s, 9);
        M_BSR(s, (s->psl >> 6) != s->r);
        break;
    case 0xbb:      /* ZBSR (*)a */
        burn(s, 9);
        M_ZBSR(s);
        break;
    case 0xbc: case 0xbd: case 0xbe:                /* BSFA,n (*)a */
        burn(s, 9);
        M_BSA(s, (s->psl >> 6) != s->r);
        break;
    case 0xbf:      /* BSXA (*)a */
        burn(s, 9);
        M_BSXA(s);
        break;
    case 0xc0:      /* NOP */
        burn(s, 6);
        break;
    case 0xc1: case 0xc2: case 0xc3:                /* STRZ,n */
        burn(s, 6);
        M_LOD(s, &s->reg[s->r], R0);
        break;
    case 0xc8: case 0xc9: case 0xca: case 0xcb:     /* STRR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        WRMEM(s, s->ea, s->reg[s->r]);
        break;
    case 0xcc: case 0xcd: case 0xce: case 0xcf:     /* STRA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        WRMEM(s, s->ea, s->reg[s->r]);
        break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:     /* RRL,n */
        burn(s, 6);
        M_RRL(s, &s->reg[s->r]);
        break;
    case 0xd4: case 0xd5: case 0xd6: case 0xd7:     /* WRTE,n v */
        burn(s, 9);
        data_w(s, ARG(s), s->reg[s->r]);
        break;
    case 0xd8: case 0xd9: case 0xda: case 0xdb:     /* BIRR,n (*)a */
        burn(s, 9);
        M_BRR(s, ++s->reg[s->r]);
        break;
    case 0xdc: case 0xdd: case 0xde: case 0xdf:     /* BIRA,n (*)a */
        burn(s, 9);
        M_BRA(s, ++s->reg[s->r]);
        break;
    case 0xe0: case 0xe1: case 0xe2: case 0xe3:     /* COMZ,n */
        burn(s, 6);
        M_COM(s, R0, s->reg[s->r]);
        break;
    case 0xe4: case 0xe5: case 0xe6: case 0xe7:     /* COMI,n v */
        burn(s, 6);
        M_COM(s, s->reg[s->r], ARG(s));
        break;
    case 0xe8: case 0xe9: case 0xea: case 0xeb:     /* COMR,n (*)a */
        burn(s, 9);
        REL_EA(s, s->page);
        M_COM(s, s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0xec: case 0xed: case 0xee: case 0xef:     /* COMA,n (*)a(,X) */
        burn(s, 12);
        ABS_EA(s);
        M_COM(s, s->reg[s->r], RDMEM(s, s->ea));
        break;
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:     /* WRTD,n */
        burn(s, 6);
        data_w(s, S2650_DATA_PORT, s->reg[s->r]);
        break;
    case 0xf4: case 0xf5: case 0xf6: case 0xf7:     /* TMI,n v */
        burn(s, 9);
        M_TMI(s, s->reg[s->r]);
        break;
    case 0xf8: case 0xf9: case 0xfa: case 0xfb:     /* BDRR,n (*)a */
        burn(s, 9);
        M_BRR(s, --s->reg[s->r]);
        break;
    case 0xfc: case 0xfd: case 0xfe: case 0xff:     /* BDRA,n (*)a */
        burn(s, 9);
        M_BRA(s, --s->reg[s->r]);
        break;
    default:        /* illegal */
        burn(s, 6);
        s->illegal_count++;
        s->illegal_pc = s->ppc;
        break;
    }
    return s->step_cycles;
}
