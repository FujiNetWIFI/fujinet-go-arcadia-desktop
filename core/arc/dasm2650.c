/* dasm2650.c -- see dasm2650.h.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * (transposed from MAME src/devices/cpu/s2650/2650dasm.cpp; copyright-holders
 * Juergen Buchmueller -- see COMPLIANCE.md)
 *
 * MAME's disassemble() is one switch that streams text and sets `flags`;
 * here it is the same switch with the same format strings. The Z80-mnemonic
 * mode (DEBUG_Z80 = 0 in MAME) is dropped. Quirks kept on purpose: operand
 * addresses take their 8K page from the address of the operand byte, not the
 * opcode (so an instruction whose opcode is the last byte of a page shows the
 * next page); opcode 0x40 is HALT and 0xC0 NOP, so ANDZ,0 / STRZ,0 never
 * print; the illegal opcodes print "****   $xx".
 */

#include <stdio.h>
#include <string.h>

#include "dasm2650.h"

static const char cc[4] = { 'z', 'p', 'm', 'a' };

/* util::sext(v, 7): the low 7 bits as a signed number */
static int sext7(int v)
{
    v &= 0x7f;
    return v & 0x40 ? v - 0x80 : v;
}

typedef struct {
    char s[48];
    size_t n;
} sbuf;

static void add(sbuf *b, const char *str)
{
    if (b->n)
        b->s[b->n++] = '+';
    size_t l = strlen(str);
    memcpy(b->s + b->n, str, l);
    b->n += l;
    b->s[b->n] = 0;
}

/* PSL */
static void imm_psl(char *out, unsigned v)
{
    sbuf b = { "", 0 };
    if (v == 0xff) {
        strcpy(out, "all");
        return;
    }
    switch (v & 0xc0) {
    case 0x40: add(&b, "p"); break;
    case 0x80: add(&b, "m"); break;
    case 0xc0: add(&b, "cc"); break;
    }
    if (v & 0x20) add(&b, "idc");
    if (v & 0x10) add(&b, "rs");
    if (v & 0x08) add(&b, "wc");
    if (v & 0x04) add(&b, "ovf");
    if (v & 0x02) add(&b, "com");
    if (v & 0x01) add(&b, "c");
    strcpy(out, b.s);
}

/* PSU */
static void imm_psu(char *out, unsigned v)
{
    sbuf b = { "", 0 };
    if (v == 0xff) {
        strcpy(out, "all");
        return;
    }
    if (v & 0x80) add(&b, "si");
    if (v & 0x40) add(&b, "fo");
    if (v & 0x20) add(&b, "ii");
    if (v & 0x10) add(&b, "4");
    if (v & 0x08) add(&b, "3");
    if (v & 0x04) add(&b, "sp2");
    if (v & 0x02) add(&b, "sp1");
    if (v & 0x01) add(&b, "sp0");
    strcpy(out, b.s);
}

/* REL: pc is the address of the operand byte */
static void rel(char *out, unsigned pc, int o)
{
    sprintf(out, "%s$%04x", (o & 0x80) ? "*" : "",
            (unsigned)(((pc & 0x6000) + ((pc + 1 + sext7(o)) & 0x1fff)) & 0xffffu));
}

static void rel0(char *out, int o)
{
    sprintf(out, "%s$%04x", (o & 0x80) ? "*" : "", (unsigned)(sext7(o) & 0x1fff));
}

uint16_t dasm2650_operand2_addr(uint16_t pc)
{
    unsigned op = (unsigned)pc + 1;     /* the first operand byte */
    return (uint16_t)((op & 0x6000) + ((op + 1) & 0x1fff));
}

/* ABS (the Signetics-mnemonic branch of MAME's) */
static void absl(char *out, int r, unsigned pc, int h, int l)
{
    unsigned a = (pc & 0x6000) + ((unsigned)(h & 0x1f) << 8) + (unsigned)l;
    switch (h >> 5) {
    case 0: sprintf(out, "%d $%04x", r, a); break;
    case 1: sprintf(out, "0 $%04x,r%d+", a, r); break;
    case 2: sprintf(out, "0 $%04x,r%d-", a, r); break;
    case 3: sprintf(out, "0 $%04x,r%d", a, r); break;
    case 4: sprintf(out, "%d *$%04x", r, a); break;
    case 5: sprintf(out, "0 *$%04x,r%d+", a, r); break;
    case 6: sprintf(out, "0 *$%04x,r%d-", a, r); break;
    default: sprintf(out, "0 *$%04x,r%d", a, r); break;
    }
}

/* ADR */
static void adr(char *out, int h, int l)
{
    unsigned a = ((unsigned)(h & 0x7f) << 8) + (unsigned)l;
    sprintf(out, "%s$%04x", (h & 0x80) ? "*" : "", a);
}

int dasm2650_flags(uint16_t pcin, const uint8_t *opcodes, char *out, size_t outsz, uint32_t *flagsout)
{
    char t[96], a[48];
    uint32_t flags = 0;
    const unsigned PC = pcin;
    unsigned pc = PC;
    const int op = opcodes[0];
    const int rv = op & 3;
    const int b1 = opcodes[1];
    const int b2 = opcodes[2];

    pc += 1;
    t[0] = 0;
    switch (op) {
    case 0x00: case 0x01: case 0x02: case 0x03:
        sprintf(t, "lodz,%d", rv);
        break;
    case 0x04: case 0x05: case 0x06: case 0x07:
        sprintf(t, "lodi,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0x08: case 0x09: case 0x0a: case 0x0b:
        rel(a, pc, b1);
        sprintf(t, "lodr,%d %s", rv, a);
        pc += 1;
        break;
    case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "loda,%s", a);
        pc += 2;
        break;
    case 0x10: case 0x11:
        sprintf(t, "****   $%02X", op);
        break;
    case 0x12:
        strcpy(t, "spsu");
        break;
    case 0x13:
        strcpy(t, "spsl");
        break;
    case 0x14: case 0x15: case 0x16: case 0x17:
        sprintf(t, "retc   %c", cc[rv]);
        flags = DASM2650_STEP_OUT | (rv != 3 ? DASM2650_STEP_COND : 0);
        break;
    case 0x18: case 0x19: case 0x1a: case 0x1b:
        rel(a, pc, b1);
        sprintf(t, "bctr,%c %s", cc[rv], a);
        pc += 1;
        if (rv != 3)
            flags = DASM2650_STEP_COND;
        break;
    case 0x1c: case 0x1d: case 0x1e: case 0x1f:
        adr(a, b1, b2);
        sprintf(t, "bcta,%c %s", cc[rv], a);
        pc += 2;
        if (rv != 3)
            flags = DASM2650_STEP_COND;
        break;
    case 0x20: case 0x21: case 0x22: case 0x23:
        sprintf(t, "eorz,%d", rv);
        break;
    case 0x24: case 0x25: case 0x26: case 0x27:
        sprintf(t, "eori,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0x28: case 0x29: case 0x2a: case 0x2b:
        rel(a, pc, b1);
        sprintf(t, "eorr,%d %s", rv, a);
        pc += 1;
        break;
    case 0x2c: case 0x2d: case 0x2e: case 0x2f:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "eora,%s", a);
        pc += 2;
        break;
    case 0x30: case 0x31: case 0x32: case 0x33:
        sprintf(t, "redc,%d", rv);
        break;
    case 0x34: case 0x35: case 0x36: case 0x37:
        sprintf(t, "rete   %c", cc[rv]);
        flags = DASM2650_STEP_OUT | (rv != 3 ? DASM2650_STEP_COND : 0);
        break;
    case 0x38: case 0x39: case 0x3a: case 0x3b:
        rel(a, pc, b1);
        sprintf(t, "bstr,%c %s", cc[rv], a);
        pc += 1;
        flags = DASM2650_STEP_OVER | (rv != 3 ? DASM2650_STEP_COND : 0);
        break;
    case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        adr(a, b1, b2);
        sprintf(t, "bsta,%c %s", cc[rv], a);
        pc += 2;
        flags = DASM2650_STEP_OVER | (rv != 3 ? DASM2650_STEP_COND : 0);
        break;
    case 0x40:
        strcpy(t, "halt");
        break;
    case 0x41: case 0x42: case 0x43:
        sprintf(t, "andz,%d", rv);
        break;
    case 0x44: case 0x45: case 0x46: case 0x47:
        sprintf(t, "andi,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0x48: case 0x49: case 0x4a: case 0x4b:
        rel(a, pc, b1);
        sprintf(t, "andr,%d %s", rv, a);
        pc += 1;
        break;
    case 0x4c: case 0x4d: case 0x4e: case 0x4f:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "anda,%s", a);
        pc += 2;
        break;
    case 0x50: case 0x51: case 0x52: case 0x53:
        sprintf(t, "rrr,%d", rv);
        break;
    case 0x54: case 0x55: case 0x56: case 0x57:
        sprintf(t, "rede,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0x58: case 0x59: case 0x5a: case 0x5b:
        rel(a, pc, b1);
        sprintf(t, "brnr,%d %s", rv, a);
        pc += 1;
        flags = DASM2650_STEP_COND;
        break;
    case 0x5c: case 0x5d: case 0x5e: case 0x5f:
        adr(a, b1, b2);
        sprintf(t, "brna,%d %s", rv, a);
        pc += 2;
        flags = DASM2650_STEP_COND;
        break;
    case 0x60: case 0x61: case 0x62: case 0x63:
        sprintf(t, "iorz,%d", rv);
        break;
    case 0x64: case 0x65: case 0x66: case 0x67:
        sprintf(t, "iori,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0x68: case 0x69: case 0x6a: case 0x6b:
        rel(a, pc, b1);
        sprintf(t, "iorr,%d %s", rv, a);
        pc += 1;
        break;
    case 0x6c: case 0x6d: case 0x6e: case 0x6f:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "iora,%s", a);
        pc += 2;
        break;
    case 0x70: case 0x71: case 0x72: case 0x73:
        sprintf(t, "redd,%d", rv);
        break;
    case 0x74:
        imm_psu(a, (unsigned)b1);
        sprintf(t, "cpsu   %s", a);
        pc += 1;
        break;
    case 0x75:
        imm_psl(a, (unsigned)b1);
        sprintf(t, "cpsl   %s", a);
        pc += 1;
        break;
    case 0x76:
        imm_psu(a, (unsigned)b1);
        sprintf(t, "ppsu   %s", a);
        pc += 1;
        break;
    case 0x77:
        imm_psl(a, (unsigned)b1);
        sprintf(t, "ppsl   %s", a);
        pc += 1;
        break;
    case 0x78: case 0x79: case 0x7a: case 0x7b:
        rel(a, pc, b1);
        sprintf(t, "bsnr,%d %s", rv, a);
        pc += 1;
        flags = DASM2650_STEP_OVER | DASM2650_STEP_COND;
        break;
    case 0x7c: case 0x7d: case 0x7e: case 0x7f:
        adr(a, b1, b2);
        sprintf(t, "bsna,%d %s", rv, a);
        pc += 2;
        flags = DASM2650_STEP_OVER | DASM2650_STEP_COND;
        break;
    case 0x80: case 0x81: case 0x82: case 0x83:
        sprintf(t, "addz,%d", rv);
        break;
    case 0x84: case 0x85: case 0x86: case 0x87:
        sprintf(t, "addi,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0x88: case 0x89: case 0x8a: case 0x8b:
        rel(a, pc, b1);
        sprintf(t, "addr,%d %s", rv, a);
        pc += 1;
        break;
    case 0x8c: case 0x8d: case 0x8e: case 0x8f:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "adda,%s", a);
        pc += 2;
        break;
    case 0x90: case 0x91:
        sprintf(t, "****   $%02X", op);
        break;
    case 0x92:
        strcpy(t, "lpsu");
        break;
    case 0x93:
        strcpy(t, "lpsl");
        break;
    case 0x94: case 0x95: case 0x96: case 0x97:
        sprintf(t, "dar,%d", rv);
        break;
    case 0x98: case 0x99: case 0x9a:
        rel(a, pc, b1);
        sprintf(t, "bcfr,%c %s", cc[rv], a);
        pc += 1;
        flags = DASM2650_STEP_COND;
        break;
    case 0x9b:
        rel0(a, b1);
        sprintf(t, "zbrr   %s", a);
        pc += 1;
        break;
    case 0x9c: case 0x9d: case 0x9e:
        adr(a, b1, b2);
        sprintf(t, "bcfa,%c %s", cc[rv], a);
        pc += 2;
        break;
    case 0x9f:
        adr(a, b1, b2);
        sprintf(t, "bxa    %s", a);
        pc += 2;
        break;
    case 0xa0: case 0xa1: case 0xa2: case 0xa3:
        sprintf(t, "subz,%d", rv);
        break;
    case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        sprintf(t, "subi,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0xa8: case 0xa9: case 0xaa: case 0xab:
        rel(a, pc, b1);
        sprintf(t, "subr,%d %s", rv, a);
        pc += 1;
        break;
    case 0xac: case 0xad: case 0xae: case 0xaf:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "suba,%s", a);
        pc += 2;
        break;
    case 0xb0: case 0xb1: case 0xb2: case 0xb3:
        sprintf(t, "wrtc,%d", rv);
        break;
    case 0xb4:
        imm_psu(a, (unsigned)b1);
        sprintf(t, "tpsu   %s", a);
        pc += 1;
        break;
    case 0xb5:
        imm_psl(a, (unsigned)b1);
        sprintf(t, "tpsl   %s", a);
        pc += 1;
        break;
    case 0xb6: case 0xb7:
        sprintf(t, "****   $%02X", op);
        break;
    case 0xb8: case 0xb9: case 0xba:
        rel(a, pc, b1);
        sprintf(t, "bsfr,%c %s", cc[rv], a);
        pc += 1;
        flags = DASM2650_STEP_OVER | DASM2650_STEP_COND;
        break;
    case 0xbb:
        rel0(a, b1);
        sprintf(t, "zbsr   %s", a);
        pc += 1;
        flags = DASM2650_STEP_OVER;
        break;
    case 0xbc: case 0xbd: case 0xbe:
        adr(a, b1, b2);
        sprintf(t, "bsfa,%c %s", cc[rv], a);
        pc += 2;
        flags = DASM2650_STEP_OVER | DASM2650_STEP_COND;
        break;
    case 0xbf:
        adr(a, b1, b2);
        sprintf(t, "bsxa   %s", a);
        pc += 2;
        flags = DASM2650_STEP_OVER;
        break;
    case 0xc0:
        strcpy(t, "nop");
        break;
    case 0xc1: case 0xc2: case 0xc3:
        sprintf(t, "strz,%d", rv);
        break;
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
        sprintf(t, "****   $%02X", op);
        break;
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
        rel(a, pc, b1);
        sprintf(t, "strr,%d %s", rv, a);
        pc += 1;
        break;
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "stra,%s", a);
        pc += 2;
        break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
        sprintf(t, "rrl,%d", rv);
        break;
    case 0xd4: case 0xd5: case 0xd6: case 0xd7:
        sprintf(t, "wrte,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
        rel(a, pc, b1);
        sprintf(t, "birr,%d %s", rv, a);
        pc += 1;
        flags = DASM2650_STEP_COND;
        break;
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
        adr(a, b1, b2);
        sprintf(t, "bira,%d %s", rv, a);
        pc += 2;
        flags = DASM2650_STEP_COND;
        break;
    case 0xe0: case 0xe1: case 0xe2: case 0xe3:
        sprintf(t, "comz,%d", rv);
        break;
    case 0xe4: case 0xe5: case 0xe6: case 0xe7:
        sprintf(t, "comi,%d $%02x", rv, b1);
        pc += 1;
        break;
    case 0xe8: case 0xe9: case 0xea: case 0xeb:
        rel(a, pc, b1);
        sprintf(t, "comr,%d %s", rv, a);
        pc += 1;
        break;
    case 0xec: case 0xed: case 0xee: case 0xef:
        absl(a, rv, pc, b1, b2);
        sprintf(t, "coma,%s", a);
        pc += 2;
        break;
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:
        sprintf(t, "wrtd,%d", rv);
        break;
    case 0xf4: case 0xf5: case 0xf6: case 0xf7:
        sprintf(t, "tmi,%d  $%02x", rv, b1);
        pc += 1;
        break;
    case 0xf8: case 0xf9: case 0xfa: case 0xfb:
        rel(a, pc, b1);
        sprintf(t, "bdrr,%d %s", rv, a);
        pc += 1;
        flags = DASM2650_STEP_COND;
        break;
    default:    /* 0xfc-0xff */
        adr(a, b1, b2);
        sprintf(t, "bdra,%d %s", rv, a);
        pc += 2;
        flags = DASM2650_STEP_COND;
        break;
    }

    if (out && outsz) {
        strncpy(out, t, outsz - 1);
        out[outsz - 1] = 0;
    }
    if (flagsout)
        *flagsout = flags | DASM2650_SUPPORTED;
    return (int)(pc - PC);
}

int dasm2650(uint16_t pc, const uint8_t *opcodes, char *out, size_t outsz)
{
    return dasm2650_flags(pc, opcodes, out, outsz, NULL);
}

int dasm2650_length(uint8_t opcode)
{
    const uint8_t b[3] = { opcode, 0, 0 };
    return dasm2650_flags(0, b, NULL, 0, NULL);
}

uint32_t dasm2650_op_flags(uint8_t opcode)
{
    const uint8_t b[3] = { opcode, 0, 0 };
    uint32_t f;
    dasm2650_flags(0, b, NULL, 0, &f);
    return f;
}

int dasm2650_target(uint16_t pc, const uint8_t *opcodes)
{
    const int op = opcodes[0];
    const int b1 = opcodes[1];
    const int b2 = opcodes[2];
    const unsigned opa = (unsigned)pc + 1;   /* the operand byte */

    switch (op & 0xfc) {
    case 0x18: case 0x38: case 0x58: case 0x78: case 0x98: case 0xb8: case 0xd8: case 0xf8:
        if (op == 0x9b || op == 0xbb) {
            if (b1 & 0x80)
                return -1;
            return sext7(b1) & 0x1fff;
        }
        if (b1 & 0x80)
            return -1;
        return (int)((opa & 0x6000) + ((opa + 1 + (unsigned)sext7(b1)) & 0x1fff));
    case 0x1c: case 0x3c: case 0x5c: case 0x7c: case 0x9c: case 0xdc: case 0xfc: case 0xbc:
        if (op == 0x9f || op == 0xbf || (b1 & 0x80))
            return -1;
        return ((b1 & 0x7f) << 8) + b2;
    default:
        return -1;
    }
}
