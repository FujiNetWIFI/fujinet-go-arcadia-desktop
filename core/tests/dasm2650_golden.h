/* dasm2650_golden.h -- MAME's own disassembly of every 2650 opcode, from
 * src/devices/cpu/s2650/2650dasm.cpp compiled standalone (tools/ab/
 * s2650_dasm_golden.cpp, run by tools/ab/gen_dasm2650_golden.sh). GENERATED;
 * dasm2650_test.c holds dasm2650.c to it.
 *
 * Each opcode is disassembled at the NV pc/operand triples of
 * dasm2650_variants (b2 is the byte MAME reads for an absolute address's
 * low half); a one-byte opcode has one text, the others NV.
 * flags is MAME's result >> 28: 8 = SUPPORTED, 4 = STEP_OUT, 2 = STEP_OVER,
 * 1 = STEP_COND. */
#define DASM2650_NV 16
static const struct { uint16_t pc; uint8_t b1, b2; } dasm2650_variants[DASM2650_NV] = {
    { 0x0123, 0x5A, 0xC3 },
    { 0x0123, 0x00, 0x00 },
    { 0x1FFE, 0x20, 0xFF },
    { 0x1FFF, 0x41, 0x12 },
    { 0x2010, 0x60, 0xA5 },
    { 0x3FFD, 0x80, 0x00 },
    { 0x4000, 0xA1, 0x7F },
    { 0x5FFF, 0xC2, 0x80 },
    { 0x6FFD, 0xE3, 0x34 },
    { 0x7FFE, 0x7F, 0xFF },
    { 0x7FFF, 0xFF, 0x01 },
    { 0x0000, 0x40, 0x9C },
    { 0x0ABC, 0x15, 0x2A },
    { 0x1000, 0x2A, 0x55 },
    { 0x6100, 0x9C, 0x33 },
    { 0x0777, 0x3F, 0xC0 },
};
static const struct { uint8_t len; uint8_t flags; const char *text[DASM2650_NV]; } dasm2650_golden[256] = {
    { 1, 8, { "lodz,0" } },   /* 00 */
    { 1, 8, { "lodz,1" } },   /* 01 */
    { 1, 8, { "lodz,2" } },   /* 02 */
    { 1, 8, { "lodz,3" } },   /* 03 */
    { 2, 8, { "lodi,0 $5a", "lodi,0 $00", "lodi,0 $20", "lodi,0 $41", "lodi,0 $60", "lodi,0 $80", "lodi,0 $a1", "lodi,0 $c2", "lodi,0 $e3", "lodi,0 $7f", "lodi,0 $ff", "lodi,0 $40", "lodi,0 $15", "lodi,0 $2a", "lodi,0 $9c", "lodi,0 $3f" } },   /* 04 */
    { 2, 8, { "lodi,1 $5a", "lodi,1 $00", "lodi,1 $20", "lodi,1 $41", "lodi,1 $60", "lodi,1 $80", "lodi,1 $a1", "lodi,1 $c2", "lodi,1 $e3", "lodi,1 $7f", "lodi,1 $ff", "lodi,1 $40", "lodi,1 $15", "lodi,1 $2a", "lodi,1 $9c", "lodi,1 $3f" } },   /* 05 */
    { 2, 8, { "lodi,2 $5a", "lodi,2 $00", "lodi,2 $20", "lodi,2 $41", "lodi,2 $60", "lodi,2 $80", "lodi,2 $a1", "lodi,2 $c2", "lodi,2 $e3", "lodi,2 $7f", "lodi,2 $ff", "lodi,2 $40", "lodi,2 $15", "lodi,2 $2a", "lodi,2 $9c", "lodi,2 $3f" } },   /* 06 */
    { 2, 8, { "lodi,3 $5a", "lodi,3 $00", "lodi,3 $20", "lodi,3 $41", "lodi,3 $60", "lodi,3 $80", "lodi,3 $a1", "lodi,3 $c2", "lodi,3 $e3", "lodi,3 $7f", "lodi,3 $ff", "lodi,3 $40", "lodi,3 $15", "lodi,3 $2a", "lodi,3 $9c", "lodi,3 $3f" } },   /* 07 */
    { 2, 8, { "lodr,0 $00ff", "lodr,0 $0125", "lodr,0 $0020", "lodr,0 $3fc2", "lodr,0 $3ff2", "lodr,0 *$3fff", "lodr,0 *$4023", "lodr,0 *$7fc3", "lodr,0 *$6fe2", "lodr,0 $7fff", "lodr,0 *$0000", "lodr,0 $1fc2", "lodr,0 $0ad3", "lodr,0 $102c", "lodr,0 *$611e", "lodr,0 $07b8" } },   /* 08 */
    { 2, 8, { "lodr,1 $00ff", "lodr,1 $0125", "lodr,1 $0020", "lodr,1 $3fc2", "lodr,1 $3ff2", "lodr,1 *$3fff", "lodr,1 *$4023", "lodr,1 *$7fc3", "lodr,1 *$6fe2", "lodr,1 $7fff", "lodr,1 *$0000", "lodr,1 $1fc2", "lodr,1 $0ad3", "lodr,1 $102c", "lodr,1 *$611e", "lodr,1 $07b8" } },   /* 09 */
    { 2, 8, { "lodr,2 $00ff", "lodr,2 $0125", "lodr,2 $0020", "lodr,2 $3fc2", "lodr,2 $3ff2", "lodr,2 *$3fff", "lodr,2 *$4023", "lodr,2 *$7fc3", "lodr,2 *$6fe2", "lodr,2 $7fff", "lodr,2 *$0000", "lodr,2 $1fc2", "lodr,2 $0ad3", "lodr,2 $102c", "lodr,2 *$611e", "lodr,2 $07b8" } },   /* 0A */
    { 2, 8, { "lodr,3 $00ff", "lodr,3 $0125", "lodr,3 $0020", "lodr,3 $3fc2", "lodr,3 $3ff2", "lodr,3 *$3fff", "lodr,3 *$4023", "lodr,3 *$7fc3", "lodr,3 *$6fe2", "lodr,3 $7fff", "lodr,3 *$0000", "lodr,3 $1fc2", "lodr,3 $0ad3", "lodr,3 $102c", "lodr,3 *$611e", "lodr,3 $07b8" } },   /* 0B */
    { 3, 8, { "loda,0 $1ac3,r0-", "loda,0 $0000", "loda,0 $00ff,r0+", "loda,0 $2112,r0-", "loda,0 $20a5,r0", "loda,0 *$2000", "loda,0 *$417f,r0+", "loda,0 *$6280,r0-", "loda,0 *$6334,r0", "loda,0 $7fff,r0", "loda,0 *$1f01,r0", "loda,0 $009c,r0-", "loda,0 $152a", "loda,0 $0a55,r0+", "loda,0 *$7c33", "loda,0 $1fc0,r0+" } },   /* 0C */
    { 3, 8, { "loda,0 $1ac3,r1-", "loda,1 $0000", "loda,0 $00ff,r1+", "loda,0 $2112,r1-", "loda,0 $20a5,r1", "loda,1 *$2000", "loda,0 *$417f,r1+", "loda,0 *$6280,r1-", "loda,0 *$6334,r1", "loda,0 $7fff,r1", "loda,0 *$1f01,r1", "loda,0 $009c,r1-", "loda,1 $152a", "loda,0 $0a55,r1+", "loda,1 *$7c33", "loda,0 $1fc0,r1+" } },   /* 0D */
    { 3, 8, { "loda,0 $1ac3,r2-", "loda,2 $0000", "loda,0 $00ff,r2+", "loda,0 $2112,r2-", "loda,0 $20a5,r2", "loda,2 *$2000", "loda,0 *$417f,r2+", "loda,0 *$6280,r2-", "loda,0 *$6334,r2", "loda,0 $7fff,r2", "loda,0 *$1f01,r2", "loda,0 $009c,r2-", "loda,2 $152a", "loda,0 $0a55,r2+", "loda,2 *$7c33", "loda,0 $1fc0,r2+" } },   /* 0E */
    { 3, 8, { "loda,0 $1ac3,r3-", "loda,3 $0000", "loda,0 $00ff,r3+", "loda,0 $2112,r3-", "loda,0 $20a5,r3", "loda,3 *$2000", "loda,0 *$417f,r3+", "loda,0 *$6280,r3-", "loda,0 *$6334,r3", "loda,0 $7fff,r3", "loda,0 *$1f01,r3", "loda,0 $009c,r3-", "loda,3 $152a", "loda,0 $0a55,r3+", "loda,3 *$7c33", "loda,0 $1fc0,r3+" } },   /* 0F */
    { 1, 8, { "****   $10" } },   /* 10 */
    { 1, 8, { "****   $11" } },   /* 11 */
    { 1, 8, { "spsu" } },   /* 12 */
    { 1, 8, { "spsl" } },   /* 13 */
    { 1, 13, { "retc   z" } },   /* 14 */
    { 1, 13, { "retc   p" } },   /* 15 */
    { 1, 13, { "retc   m" } },   /* 16 */
    { 1, 12, { "retc   a" } },   /* 17 */
    { 2, 9, { "bctr,z $00ff", "bctr,z $0125", "bctr,z $0020", "bctr,z $3fc2", "bctr,z $3ff2", "bctr,z *$3fff", "bctr,z *$4023", "bctr,z *$7fc3", "bctr,z *$6fe2", "bctr,z $7fff", "bctr,z *$0000", "bctr,z $1fc2", "bctr,z $0ad3", "bctr,z $102c", "bctr,z *$611e", "bctr,z $07b8" } },   /* 18 */
    { 2, 9, { "bctr,p $00ff", "bctr,p $0125", "bctr,p $0020", "bctr,p $3fc2", "bctr,p $3ff2", "bctr,p *$3fff", "bctr,p *$4023", "bctr,p *$7fc3", "bctr,p *$6fe2", "bctr,p $7fff", "bctr,p *$0000", "bctr,p $1fc2", "bctr,p $0ad3", "bctr,p $102c", "bctr,p *$611e", "bctr,p $07b8" } },   /* 19 */
    { 2, 9, { "bctr,m $00ff", "bctr,m $0125", "bctr,m $0020", "bctr,m $3fc2", "bctr,m $3ff2", "bctr,m *$3fff", "bctr,m *$4023", "bctr,m *$7fc3", "bctr,m *$6fe2", "bctr,m $7fff", "bctr,m *$0000", "bctr,m $1fc2", "bctr,m $0ad3", "bctr,m $102c", "bctr,m *$611e", "bctr,m $07b8" } },   /* 1A */
    { 2, 8, { "bctr,a $00ff", "bctr,a $0125", "bctr,a $0020", "bctr,a $3fc2", "bctr,a $3ff2", "bctr,a *$3fff", "bctr,a *$4023", "bctr,a *$7fc3", "bctr,a *$6fe2", "bctr,a $7fff", "bctr,a *$0000", "bctr,a $1fc2", "bctr,a $0ad3", "bctr,a $102c", "bctr,a *$611e", "bctr,a $07b8" } },   /* 1B */
    { 3, 9, { "bcta,z $5ac3", "bcta,z $0000", "bcta,z $20ff", "bcta,z $4112", "bcta,z $60a5", "bcta,z *$0000", "bcta,z *$217f", "bcta,z *$4280", "bcta,z *$6334", "bcta,z $7fff", "bcta,z *$7f01", "bcta,z $409c", "bcta,z $152a", "bcta,z $2a55", "bcta,z *$1c33", "bcta,z $3fc0" } },   /* 1C */
    { 3, 9, { "bcta,p $5ac3", "bcta,p $0000", "bcta,p $20ff", "bcta,p $4112", "bcta,p $60a5", "bcta,p *$0000", "bcta,p *$217f", "bcta,p *$4280", "bcta,p *$6334", "bcta,p $7fff", "bcta,p *$7f01", "bcta,p $409c", "bcta,p $152a", "bcta,p $2a55", "bcta,p *$1c33", "bcta,p $3fc0" } },   /* 1D */
    { 3, 9, { "bcta,m $5ac3", "bcta,m $0000", "bcta,m $20ff", "bcta,m $4112", "bcta,m $60a5", "bcta,m *$0000", "bcta,m *$217f", "bcta,m *$4280", "bcta,m *$6334", "bcta,m $7fff", "bcta,m *$7f01", "bcta,m $409c", "bcta,m $152a", "bcta,m $2a55", "bcta,m *$1c33", "bcta,m $3fc0" } },   /* 1E */
    { 3, 8, { "bcta,a $5ac3", "bcta,a $0000", "bcta,a $20ff", "bcta,a $4112", "bcta,a $60a5", "bcta,a *$0000", "bcta,a *$217f", "bcta,a *$4280", "bcta,a *$6334", "bcta,a $7fff", "bcta,a *$7f01", "bcta,a $409c", "bcta,a $152a", "bcta,a $2a55", "bcta,a *$1c33", "bcta,a $3fc0" } },   /* 1F */
    { 1, 8, { "eorz,0" } },   /* 20 */
    { 1, 8, { "eorz,1" } },   /* 21 */
    { 1, 8, { "eorz,2" } },   /* 22 */
    { 1, 8, { "eorz,3" } },   /* 23 */
    { 2, 8, { "eori,0 $5a", "eori,0 $00", "eori,0 $20", "eori,0 $41", "eori,0 $60", "eori,0 $80", "eori,0 $a1", "eori,0 $c2", "eori,0 $e3", "eori,0 $7f", "eori,0 $ff", "eori,0 $40", "eori,0 $15", "eori,0 $2a", "eori,0 $9c", "eori,0 $3f" } },   /* 24 */
    { 2, 8, { "eori,1 $5a", "eori,1 $00", "eori,1 $20", "eori,1 $41", "eori,1 $60", "eori,1 $80", "eori,1 $a1", "eori,1 $c2", "eori,1 $e3", "eori,1 $7f", "eori,1 $ff", "eori,1 $40", "eori,1 $15", "eori,1 $2a", "eori,1 $9c", "eori,1 $3f" } },   /* 25 */
    { 2, 8, { "eori,2 $5a", "eori,2 $00", "eori,2 $20", "eori,2 $41", "eori,2 $60", "eori,2 $80", "eori,2 $a1", "eori,2 $c2", "eori,2 $e3", "eori,2 $7f", "eori,2 $ff", "eori,2 $40", "eori,2 $15", "eori,2 $2a", "eori,2 $9c", "eori,2 $3f" } },   /* 26 */
    { 2, 8, { "eori,3 $5a", "eori,3 $00", "eori,3 $20", "eori,3 $41", "eori,3 $60", "eori,3 $80", "eori,3 $a1", "eori,3 $c2", "eori,3 $e3", "eori,3 $7f", "eori,3 $ff", "eori,3 $40", "eori,3 $15", "eori,3 $2a", "eori,3 $9c", "eori,3 $3f" } },   /* 27 */
    { 2, 8, { "eorr,0 $00ff", "eorr,0 $0125", "eorr,0 $0020", "eorr,0 $3fc2", "eorr,0 $3ff2", "eorr,0 *$3fff", "eorr,0 *$4023", "eorr,0 *$7fc3", "eorr,0 *$6fe2", "eorr,0 $7fff", "eorr,0 *$0000", "eorr,0 $1fc2", "eorr,0 $0ad3", "eorr,0 $102c", "eorr,0 *$611e", "eorr,0 $07b8" } },   /* 28 */
    { 2, 8, { "eorr,1 $00ff", "eorr,1 $0125", "eorr,1 $0020", "eorr,1 $3fc2", "eorr,1 $3ff2", "eorr,1 *$3fff", "eorr,1 *$4023", "eorr,1 *$7fc3", "eorr,1 *$6fe2", "eorr,1 $7fff", "eorr,1 *$0000", "eorr,1 $1fc2", "eorr,1 $0ad3", "eorr,1 $102c", "eorr,1 *$611e", "eorr,1 $07b8" } },   /* 29 */
    { 2, 8, { "eorr,2 $00ff", "eorr,2 $0125", "eorr,2 $0020", "eorr,2 $3fc2", "eorr,2 $3ff2", "eorr,2 *$3fff", "eorr,2 *$4023", "eorr,2 *$7fc3", "eorr,2 *$6fe2", "eorr,2 $7fff", "eorr,2 *$0000", "eorr,2 $1fc2", "eorr,2 $0ad3", "eorr,2 $102c", "eorr,2 *$611e", "eorr,2 $07b8" } },   /* 2A */
    { 2, 8, { "eorr,3 $00ff", "eorr,3 $0125", "eorr,3 $0020", "eorr,3 $3fc2", "eorr,3 $3ff2", "eorr,3 *$3fff", "eorr,3 *$4023", "eorr,3 *$7fc3", "eorr,3 *$6fe2", "eorr,3 $7fff", "eorr,3 *$0000", "eorr,3 $1fc2", "eorr,3 $0ad3", "eorr,3 $102c", "eorr,3 *$611e", "eorr,3 $07b8" } },   /* 2B */
    { 3, 8, { "eora,0 $1ac3,r0-", "eora,0 $0000", "eora,0 $00ff,r0+", "eora,0 $2112,r0-", "eora,0 $20a5,r0", "eora,0 *$2000", "eora,0 *$417f,r0+", "eora,0 *$6280,r0-", "eora,0 *$6334,r0", "eora,0 $7fff,r0", "eora,0 *$1f01,r0", "eora,0 $009c,r0-", "eora,0 $152a", "eora,0 $0a55,r0+", "eora,0 *$7c33", "eora,0 $1fc0,r0+" } },   /* 2C */
    { 3, 8, { "eora,0 $1ac3,r1-", "eora,1 $0000", "eora,0 $00ff,r1+", "eora,0 $2112,r1-", "eora,0 $20a5,r1", "eora,1 *$2000", "eora,0 *$417f,r1+", "eora,0 *$6280,r1-", "eora,0 *$6334,r1", "eora,0 $7fff,r1", "eora,0 *$1f01,r1", "eora,0 $009c,r1-", "eora,1 $152a", "eora,0 $0a55,r1+", "eora,1 *$7c33", "eora,0 $1fc0,r1+" } },   /* 2D */
    { 3, 8, { "eora,0 $1ac3,r2-", "eora,2 $0000", "eora,0 $00ff,r2+", "eora,0 $2112,r2-", "eora,0 $20a5,r2", "eora,2 *$2000", "eora,0 *$417f,r2+", "eora,0 *$6280,r2-", "eora,0 *$6334,r2", "eora,0 $7fff,r2", "eora,0 *$1f01,r2", "eora,0 $009c,r2-", "eora,2 $152a", "eora,0 $0a55,r2+", "eora,2 *$7c33", "eora,0 $1fc0,r2+" } },   /* 2E */
    { 3, 8, { "eora,0 $1ac3,r3-", "eora,3 $0000", "eora,0 $00ff,r3+", "eora,0 $2112,r3-", "eora,0 $20a5,r3", "eora,3 *$2000", "eora,0 *$417f,r3+", "eora,0 *$6280,r3-", "eora,0 *$6334,r3", "eora,0 $7fff,r3", "eora,0 *$1f01,r3", "eora,0 $009c,r3-", "eora,3 $152a", "eora,0 $0a55,r3+", "eora,3 *$7c33", "eora,0 $1fc0,r3+" } },   /* 2F */
    { 1, 8, { "redc,0" } },   /* 30 */
    { 1, 8, { "redc,1" } },   /* 31 */
    { 1, 8, { "redc,2" } },   /* 32 */
    { 1, 8, { "redc,3" } },   /* 33 */
    { 1, 13, { "rete   z" } },   /* 34 */
    { 1, 13, { "rete   p" } },   /* 35 */
    { 1, 13, { "rete   m" } },   /* 36 */
    { 1, 12, { "rete   a" } },   /* 37 */
    { 2, 11, { "bstr,z $00ff", "bstr,z $0125", "bstr,z $0020", "bstr,z $3fc2", "bstr,z $3ff2", "bstr,z *$3fff", "bstr,z *$4023", "bstr,z *$7fc3", "bstr,z *$6fe2", "bstr,z $7fff", "bstr,z *$0000", "bstr,z $1fc2", "bstr,z $0ad3", "bstr,z $102c", "bstr,z *$611e", "bstr,z $07b8" } },   /* 38 */
    { 2, 11, { "bstr,p $00ff", "bstr,p $0125", "bstr,p $0020", "bstr,p $3fc2", "bstr,p $3ff2", "bstr,p *$3fff", "bstr,p *$4023", "bstr,p *$7fc3", "bstr,p *$6fe2", "bstr,p $7fff", "bstr,p *$0000", "bstr,p $1fc2", "bstr,p $0ad3", "bstr,p $102c", "bstr,p *$611e", "bstr,p $07b8" } },   /* 39 */
    { 2, 11, { "bstr,m $00ff", "bstr,m $0125", "bstr,m $0020", "bstr,m $3fc2", "bstr,m $3ff2", "bstr,m *$3fff", "bstr,m *$4023", "bstr,m *$7fc3", "bstr,m *$6fe2", "bstr,m $7fff", "bstr,m *$0000", "bstr,m $1fc2", "bstr,m $0ad3", "bstr,m $102c", "bstr,m *$611e", "bstr,m $07b8" } },   /* 3A */
    { 2, 10, { "bstr,a $00ff", "bstr,a $0125", "bstr,a $0020", "bstr,a $3fc2", "bstr,a $3ff2", "bstr,a *$3fff", "bstr,a *$4023", "bstr,a *$7fc3", "bstr,a *$6fe2", "bstr,a $7fff", "bstr,a *$0000", "bstr,a $1fc2", "bstr,a $0ad3", "bstr,a $102c", "bstr,a *$611e", "bstr,a $07b8" } },   /* 3B */
    { 3, 11, { "bsta,z $5ac3", "bsta,z $0000", "bsta,z $20ff", "bsta,z $4112", "bsta,z $60a5", "bsta,z *$0000", "bsta,z *$217f", "bsta,z *$4280", "bsta,z *$6334", "bsta,z $7fff", "bsta,z *$7f01", "bsta,z $409c", "bsta,z $152a", "bsta,z $2a55", "bsta,z *$1c33", "bsta,z $3fc0" } },   /* 3C */
    { 3, 11, { "bsta,p $5ac3", "bsta,p $0000", "bsta,p $20ff", "bsta,p $4112", "bsta,p $60a5", "bsta,p *$0000", "bsta,p *$217f", "bsta,p *$4280", "bsta,p *$6334", "bsta,p $7fff", "bsta,p *$7f01", "bsta,p $409c", "bsta,p $152a", "bsta,p $2a55", "bsta,p *$1c33", "bsta,p $3fc0" } },   /* 3D */
    { 3, 11, { "bsta,m $5ac3", "bsta,m $0000", "bsta,m $20ff", "bsta,m $4112", "bsta,m $60a5", "bsta,m *$0000", "bsta,m *$217f", "bsta,m *$4280", "bsta,m *$6334", "bsta,m $7fff", "bsta,m *$7f01", "bsta,m $409c", "bsta,m $152a", "bsta,m $2a55", "bsta,m *$1c33", "bsta,m $3fc0" } },   /* 3E */
    { 3, 10, { "bsta,a $5ac3", "bsta,a $0000", "bsta,a $20ff", "bsta,a $4112", "bsta,a $60a5", "bsta,a *$0000", "bsta,a *$217f", "bsta,a *$4280", "bsta,a *$6334", "bsta,a $7fff", "bsta,a *$7f01", "bsta,a $409c", "bsta,a $152a", "bsta,a $2a55", "bsta,a *$1c33", "bsta,a $3fc0" } },   /* 3F */
    { 1, 8, { "halt" } },   /* 40 */
    { 1, 8, { "andz,1" } },   /* 41 */
    { 1, 8, { "andz,2" } },   /* 42 */
    { 1, 8, { "andz,3" } },   /* 43 */
    { 2, 8, { "andi,0 $5a", "andi,0 $00", "andi,0 $20", "andi,0 $41", "andi,0 $60", "andi,0 $80", "andi,0 $a1", "andi,0 $c2", "andi,0 $e3", "andi,0 $7f", "andi,0 $ff", "andi,0 $40", "andi,0 $15", "andi,0 $2a", "andi,0 $9c", "andi,0 $3f" } },   /* 44 */
    { 2, 8, { "andi,1 $5a", "andi,1 $00", "andi,1 $20", "andi,1 $41", "andi,1 $60", "andi,1 $80", "andi,1 $a1", "andi,1 $c2", "andi,1 $e3", "andi,1 $7f", "andi,1 $ff", "andi,1 $40", "andi,1 $15", "andi,1 $2a", "andi,1 $9c", "andi,1 $3f" } },   /* 45 */
    { 2, 8, { "andi,2 $5a", "andi,2 $00", "andi,2 $20", "andi,2 $41", "andi,2 $60", "andi,2 $80", "andi,2 $a1", "andi,2 $c2", "andi,2 $e3", "andi,2 $7f", "andi,2 $ff", "andi,2 $40", "andi,2 $15", "andi,2 $2a", "andi,2 $9c", "andi,2 $3f" } },   /* 46 */
    { 2, 8, { "andi,3 $5a", "andi,3 $00", "andi,3 $20", "andi,3 $41", "andi,3 $60", "andi,3 $80", "andi,3 $a1", "andi,3 $c2", "andi,3 $e3", "andi,3 $7f", "andi,3 $ff", "andi,3 $40", "andi,3 $15", "andi,3 $2a", "andi,3 $9c", "andi,3 $3f" } },   /* 47 */
    { 2, 8, { "andr,0 $00ff", "andr,0 $0125", "andr,0 $0020", "andr,0 $3fc2", "andr,0 $3ff2", "andr,0 *$3fff", "andr,0 *$4023", "andr,0 *$7fc3", "andr,0 *$6fe2", "andr,0 $7fff", "andr,0 *$0000", "andr,0 $1fc2", "andr,0 $0ad3", "andr,0 $102c", "andr,0 *$611e", "andr,0 $07b8" } },   /* 48 */
    { 2, 8, { "andr,1 $00ff", "andr,1 $0125", "andr,1 $0020", "andr,1 $3fc2", "andr,1 $3ff2", "andr,1 *$3fff", "andr,1 *$4023", "andr,1 *$7fc3", "andr,1 *$6fe2", "andr,1 $7fff", "andr,1 *$0000", "andr,1 $1fc2", "andr,1 $0ad3", "andr,1 $102c", "andr,1 *$611e", "andr,1 $07b8" } },   /* 49 */
    { 2, 8, { "andr,2 $00ff", "andr,2 $0125", "andr,2 $0020", "andr,2 $3fc2", "andr,2 $3ff2", "andr,2 *$3fff", "andr,2 *$4023", "andr,2 *$7fc3", "andr,2 *$6fe2", "andr,2 $7fff", "andr,2 *$0000", "andr,2 $1fc2", "andr,2 $0ad3", "andr,2 $102c", "andr,2 *$611e", "andr,2 $07b8" } },   /* 4A */
    { 2, 8, { "andr,3 $00ff", "andr,3 $0125", "andr,3 $0020", "andr,3 $3fc2", "andr,3 $3ff2", "andr,3 *$3fff", "andr,3 *$4023", "andr,3 *$7fc3", "andr,3 *$6fe2", "andr,3 $7fff", "andr,3 *$0000", "andr,3 $1fc2", "andr,3 $0ad3", "andr,3 $102c", "andr,3 *$611e", "andr,3 $07b8" } },   /* 4B */
    { 3, 8, { "anda,0 $1ac3,r0-", "anda,0 $0000", "anda,0 $00ff,r0+", "anda,0 $2112,r0-", "anda,0 $20a5,r0", "anda,0 *$2000", "anda,0 *$417f,r0+", "anda,0 *$6280,r0-", "anda,0 *$6334,r0", "anda,0 $7fff,r0", "anda,0 *$1f01,r0", "anda,0 $009c,r0-", "anda,0 $152a", "anda,0 $0a55,r0+", "anda,0 *$7c33", "anda,0 $1fc0,r0+" } },   /* 4C */
    { 3, 8, { "anda,0 $1ac3,r1-", "anda,1 $0000", "anda,0 $00ff,r1+", "anda,0 $2112,r1-", "anda,0 $20a5,r1", "anda,1 *$2000", "anda,0 *$417f,r1+", "anda,0 *$6280,r1-", "anda,0 *$6334,r1", "anda,0 $7fff,r1", "anda,0 *$1f01,r1", "anda,0 $009c,r1-", "anda,1 $152a", "anda,0 $0a55,r1+", "anda,1 *$7c33", "anda,0 $1fc0,r1+" } },   /* 4D */
    { 3, 8, { "anda,0 $1ac3,r2-", "anda,2 $0000", "anda,0 $00ff,r2+", "anda,0 $2112,r2-", "anda,0 $20a5,r2", "anda,2 *$2000", "anda,0 *$417f,r2+", "anda,0 *$6280,r2-", "anda,0 *$6334,r2", "anda,0 $7fff,r2", "anda,0 *$1f01,r2", "anda,0 $009c,r2-", "anda,2 $152a", "anda,0 $0a55,r2+", "anda,2 *$7c33", "anda,0 $1fc0,r2+" } },   /* 4E */
    { 3, 8, { "anda,0 $1ac3,r3-", "anda,3 $0000", "anda,0 $00ff,r3+", "anda,0 $2112,r3-", "anda,0 $20a5,r3", "anda,3 *$2000", "anda,0 *$417f,r3+", "anda,0 *$6280,r3-", "anda,0 *$6334,r3", "anda,0 $7fff,r3", "anda,0 *$1f01,r3", "anda,0 $009c,r3-", "anda,3 $152a", "anda,0 $0a55,r3+", "anda,3 *$7c33", "anda,0 $1fc0,r3+" } },   /* 4F */
    { 1, 8, { "rrr,0" } },   /* 50 */
    { 1, 8, { "rrr,1" } },   /* 51 */
    { 1, 8, { "rrr,2" } },   /* 52 */
    { 1, 8, { "rrr,3" } },   /* 53 */
    { 2, 8, { "rede,0 $5a", "rede,0 $00", "rede,0 $20", "rede,0 $41", "rede,0 $60", "rede,0 $80", "rede,0 $a1", "rede,0 $c2", "rede,0 $e3", "rede,0 $7f", "rede,0 $ff", "rede,0 $40", "rede,0 $15", "rede,0 $2a", "rede,0 $9c", "rede,0 $3f" } },   /* 54 */
    { 2, 8, { "rede,1 $5a", "rede,1 $00", "rede,1 $20", "rede,1 $41", "rede,1 $60", "rede,1 $80", "rede,1 $a1", "rede,1 $c2", "rede,1 $e3", "rede,1 $7f", "rede,1 $ff", "rede,1 $40", "rede,1 $15", "rede,1 $2a", "rede,1 $9c", "rede,1 $3f" } },   /* 55 */
    { 2, 8, { "rede,2 $5a", "rede,2 $00", "rede,2 $20", "rede,2 $41", "rede,2 $60", "rede,2 $80", "rede,2 $a1", "rede,2 $c2", "rede,2 $e3", "rede,2 $7f", "rede,2 $ff", "rede,2 $40", "rede,2 $15", "rede,2 $2a", "rede,2 $9c", "rede,2 $3f" } },   /* 56 */
    { 2, 8, { "rede,3 $5a", "rede,3 $00", "rede,3 $20", "rede,3 $41", "rede,3 $60", "rede,3 $80", "rede,3 $a1", "rede,3 $c2", "rede,3 $e3", "rede,3 $7f", "rede,3 $ff", "rede,3 $40", "rede,3 $15", "rede,3 $2a", "rede,3 $9c", "rede,3 $3f" } },   /* 57 */
    { 2, 9, { "brnr,0 $00ff", "brnr,0 $0125", "brnr,0 $0020", "brnr,0 $3fc2", "brnr,0 $3ff2", "brnr,0 *$3fff", "brnr,0 *$4023", "brnr,0 *$7fc3", "brnr,0 *$6fe2", "brnr,0 $7fff", "brnr,0 *$0000", "brnr,0 $1fc2", "brnr,0 $0ad3", "brnr,0 $102c", "brnr,0 *$611e", "brnr,0 $07b8" } },   /* 58 */
    { 2, 9, { "brnr,1 $00ff", "brnr,1 $0125", "brnr,1 $0020", "brnr,1 $3fc2", "brnr,1 $3ff2", "brnr,1 *$3fff", "brnr,1 *$4023", "brnr,1 *$7fc3", "brnr,1 *$6fe2", "brnr,1 $7fff", "brnr,1 *$0000", "brnr,1 $1fc2", "brnr,1 $0ad3", "brnr,1 $102c", "brnr,1 *$611e", "brnr,1 $07b8" } },   /* 59 */
    { 2, 9, { "brnr,2 $00ff", "brnr,2 $0125", "brnr,2 $0020", "brnr,2 $3fc2", "brnr,2 $3ff2", "brnr,2 *$3fff", "brnr,2 *$4023", "brnr,2 *$7fc3", "brnr,2 *$6fe2", "brnr,2 $7fff", "brnr,2 *$0000", "brnr,2 $1fc2", "brnr,2 $0ad3", "brnr,2 $102c", "brnr,2 *$611e", "brnr,2 $07b8" } },   /* 5A */
    { 2, 9, { "brnr,3 $00ff", "brnr,3 $0125", "brnr,3 $0020", "brnr,3 $3fc2", "brnr,3 $3ff2", "brnr,3 *$3fff", "brnr,3 *$4023", "brnr,3 *$7fc3", "brnr,3 *$6fe2", "brnr,3 $7fff", "brnr,3 *$0000", "brnr,3 $1fc2", "brnr,3 $0ad3", "brnr,3 $102c", "brnr,3 *$611e", "brnr,3 $07b8" } },   /* 5B */
    { 3, 9, { "brna,0 $5ac3", "brna,0 $0000", "brna,0 $20ff", "brna,0 $4112", "brna,0 $60a5", "brna,0 *$0000", "brna,0 *$217f", "brna,0 *$4280", "brna,0 *$6334", "brna,0 $7fff", "brna,0 *$7f01", "brna,0 $409c", "brna,0 $152a", "brna,0 $2a55", "brna,0 *$1c33", "brna,0 $3fc0" } },   /* 5C */
    { 3, 9, { "brna,1 $5ac3", "brna,1 $0000", "brna,1 $20ff", "brna,1 $4112", "brna,1 $60a5", "brna,1 *$0000", "brna,1 *$217f", "brna,1 *$4280", "brna,1 *$6334", "brna,1 $7fff", "brna,1 *$7f01", "brna,1 $409c", "brna,1 $152a", "brna,1 $2a55", "brna,1 *$1c33", "brna,1 $3fc0" } },   /* 5D */
    { 3, 9, { "brna,2 $5ac3", "brna,2 $0000", "brna,2 $20ff", "brna,2 $4112", "brna,2 $60a5", "brna,2 *$0000", "brna,2 *$217f", "brna,2 *$4280", "brna,2 *$6334", "brna,2 $7fff", "brna,2 *$7f01", "brna,2 $409c", "brna,2 $152a", "brna,2 $2a55", "brna,2 *$1c33", "brna,2 $3fc0" } },   /* 5E */
    { 3, 9, { "brna,3 $5ac3", "brna,3 $0000", "brna,3 $20ff", "brna,3 $4112", "brna,3 $60a5", "brna,3 *$0000", "brna,3 *$217f", "brna,3 *$4280", "brna,3 *$6334", "brna,3 $7fff", "brna,3 *$7f01", "brna,3 $409c", "brna,3 $152a", "brna,3 $2a55", "brna,3 *$1c33", "brna,3 $3fc0" } },   /* 5F */
    { 1, 8, { "iorz,0" } },   /* 60 */
    { 1, 8, { "iorz,1" } },   /* 61 */
    { 1, 8, { "iorz,2" } },   /* 62 */
    { 1, 8, { "iorz,3" } },   /* 63 */
    { 2, 8, { "iori,0 $5a", "iori,0 $00", "iori,0 $20", "iori,0 $41", "iori,0 $60", "iori,0 $80", "iori,0 $a1", "iori,0 $c2", "iori,0 $e3", "iori,0 $7f", "iori,0 $ff", "iori,0 $40", "iori,0 $15", "iori,0 $2a", "iori,0 $9c", "iori,0 $3f" } },   /* 64 */
    { 2, 8, { "iori,1 $5a", "iori,1 $00", "iori,1 $20", "iori,1 $41", "iori,1 $60", "iori,1 $80", "iori,1 $a1", "iori,1 $c2", "iori,1 $e3", "iori,1 $7f", "iori,1 $ff", "iori,1 $40", "iori,1 $15", "iori,1 $2a", "iori,1 $9c", "iori,1 $3f" } },   /* 65 */
    { 2, 8, { "iori,2 $5a", "iori,2 $00", "iori,2 $20", "iori,2 $41", "iori,2 $60", "iori,2 $80", "iori,2 $a1", "iori,2 $c2", "iori,2 $e3", "iori,2 $7f", "iori,2 $ff", "iori,2 $40", "iori,2 $15", "iori,2 $2a", "iori,2 $9c", "iori,2 $3f" } },   /* 66 */
    { 2, 8, { "iori,3 $5a", "iori,3 $00", "iori,3 $20", "iori,3 $41", "iori,3 $60", "iori,3 $80", "iori,3 $a1", "iori,3 $c2", "iori,3 $e3", "iori,3 $7f", "iori,3 $ff", "iori,3 $40", "iori,3 $15", "iori,3 $2a", "iori,3 $9c", "iori,3 $3f" } },   /* 67 */
    { 2, 8, { "iorr,0 $00ff", "iorr,0 $0125", "iorr,0 $0020", "iorr,0 $3fc2", "iorr,0 $3ff2", "iorr,0 *$3fff", "iorr,0 *$4023", "iorr,0 *$7fc3", "iorr,0 *$6fe2", "iorr,0 $7fff", "iorr,0 *$0000", "iorr,0 $1fc2", "iorr,0 $0ad3", "iorr,0 $102c", "iorr,0 *$611e", "iorr,0 $07b8" } },   /* 68 */
    { 2, 8, { "iorr,1 $00ff", "iorr,1 $0125", "iorr,1 $0020", "iorr,1 $3fc2", "iorr,1 $3ff2", "iorr,1 *$3fff", "iorr,1 *$4023", "iorr,1 *$7fc3", "iorr,1 *$6fe2", "iorr,1 $7fff", "iorr,1 *$0000", "iorr,1 $1fc2", "iorr,1 $0ad3", "iorr,1 $102c", "iorr,1 *$611e", "iorr,1 $07b8" } },   /* 69 */
    { 2, 8, { "iorr,2 $00ff", "iorr,2 $0125", "iorr,2 $0020", "iorr,2 $3fc2", "iorr,2 $3ff2", "iorr,2 *$3fff", "iorr,2 *$4023", "iorr,2 *$7fc3", "iorr,2 *$6fe2", "iorr,2 $7fff", "iorr,2 *$0000", "iorr,2 $1fc2", "iorr,2 $0ad3", "iorr,2 $102c", "iorr,2 *$611e", "iorr,2 $07b8" } },   /* 6A */
    { 2, 8, { "iorr,3 $00ff", "iorr,3 $0125", "iorr,3 $0020", "iorr,3 $3fc2", "iorr,3 $3ff2", "iorr,3 *$3fff", "iorr,3 *$4023", "iorr,3 *$7fc3", "iorr,3 *$6fe2", "iorr,3 $7fff", "iorr,3 *$0000", "iorr,3 $1fc2", "iorr,3 $0ad3", "iorr,3 $102c", "iorr,3 *$611e", "iorr,3 $07b8" } },   /* 6B */
    { 3, 8, { "iora,0 $1ac3,r0-", "iora,0 $0000", "iora,0 $00ff,r0+", "iora,0 $2112,r0-", "iora,0 $20a5,r0", "iora,0 *$2000", "iora,0 *$417f,r0+", "iora,0 *$6280,r0-", "iora,0 *$6334,r0", "iora,0 $7fff,r0", "iora,0 *$1f01,r0", "iora,0 $009c,r0-", "iora,0 $152a", "iora,0 $0a55,r0+", "iora,0 *$7c33", "iora,0 $1fc0,r0+" } },   /* 6C */
    { 3, 8, { "iora,0 $1ac3,r1-", "iora,1 $0000", "iora,0 $00ff,r1+", "iora,0 $2112,r1-", "iora,0 $20a5,r1", "iora,1 *$2000", "iora,0 *$417f,r1+", "iora,0 *$6280,r1-", "iora,0 *$6334,r1", "iora,0 $7fff,r1", "iora,0 *$1f01,r1", "iora,0 $009c,r1-", "iora,1 $152a", "iora,0 $0a55,r1+", "iora,1 *$7c33", "iora,0 $1fc0,r1+" } },   /* 6D */
    { 3, 8, { "iora,0 $1ac3,r2-", "iora,2 $0000", "iora,0 $00ff,r2+", "iora,0 $2112,r2-", "iora,0 $20a5,r2", "iora,2 *$2000", "iora,0 *$417f,r2+", "iora,0 *$6280,r2-", "iora,0 *$6334,r2", "iora,0 $7fff,r2", "iora,0 *$1f01,r2", "iora,0 $009c,r2-", "iora,2 $152a", "iora,0 $0a55,r2+", "iora,2 *$7c33", "iora,0 $1fc0,r2+" } },   /* 6E */
    { 3, 8, { "iora,0 $1ac3,r3-", "iora,3 $0000", "iora,0 $00ff,r3+", "iora,0 $2112,r3-", "iora,0 $20a5,r3", "iora,3 *$2000", "iora,0 *$417f,r3+", "iora,0 *$6280,r3-", "iora,0 *$6334,r3", "iora,0 $7fff,r3", "iora,0 *$1f01,r3", "iora,0 $009c,r3-", "iora,3 $152a", "iora,0 $0a55,r3+", "iora,3 *$7c33", "iora,0 $1fc0,r3+" } },   /* 6F */
    { 1, 8, { "redd,0" } },   /* 70 */
    { 1, 8, { "redd,1" } },   /* 71 */
    { 1, 8, { "redd,2" } },   /* 72 */
    { 1, 8, { "redd,3" } },   /* 73 */
    { 2, 8, { "cpsu   fo+4+3+sp1", "cpsu   ", "cpsu   ii", "cpsu   fo+sp0", "cpsu   fo+ii", "cpsu   si", "cpsu   si+ii+sp0", "cpsu   si+fo+sp1", "cpsu   si+fo+ii+sp1+sp0", "cpsu   fo+ii+4+3+sp2+sp1+sp0", "cpsu   all", "cpsu   fo", "cpsu   4+sp2+sp0", "cpsu   ii+3+sp1", "cpsu   si+4+3+sp2", "cpsu   ii+4+3+sp2+sp1+sp0" } },   /* 74 */
    { 2, 8, { "cpsl   p+rs+wc+com", "cpsl   ", "cpsl   idc", "cpsl   p+c", "cpsl   p+idc", "cpsl   m", "cpsl   m+idc+c", "cpsl   cc+com", "cpsl   cc+idc+com+c", "cpsl   p+idc+rs+wc+ovf+com+c", "cpsl   all", "cpsl   p", "cpsl   rs+ovf+c", "cpsl   idc+wc+com", "cpsl   m+rs+wc+ovf", "cpsl   idc+rs+wc+ovf+com+c" } },   /* 75 */
    { 2, 8, { "ppsu   fo+4+3+sp1", "ppsu   ", "ppsu   ii", "ppsu   fo+sp0", "ppsu   fo+ii", "ppsu   si", "ppsu   si+ii+sp0", "ppsu   si+fo+sp1", "ppsu   si+fo+ii+sp1+sp0", "ppsu   fo+ii+4+3+sp2+sp1+sp0", "ppsu   all", "ppsu   fo", "ppsu   4+sp2+sp0", "ppsu   ii+3+sp1", "ppsu   si+4+3+sp2", "ppsu   ii+4+3+sp2+sp1+sp0" } },   /* 76 */
    { 2, 8, { "ppsl   p+rs+wc+com", "ppsl   ", "ppsl   idc", "ppsl   p+c", "ppsl   p+idc", "ppsl   m", "ppsl   m+idc+c", "ppsl   cc+com", "ppsl   cc+idc+com+c", "ppsl   p+idc+rs+wc+ovf+com+c", "ppsl   all", "ppsl   p", "ppsl   rs+ovf+c", "ppsl   idc+wc+com", "ppsl   m+rs+wc+ovf", "ppsl   idc+rs+wc+ovf+com+c" } },   /* 77 */
    { 2, 11, { "bsnr,0 $00ff", "bsnr,0 $0125", "bsnr,0 $0020", "bsnr,0 $3fc2", "bsnr,0 $3ff2", "bsnr,0 *$3fff", "bsnr,0 *$4023", "bsnr,0 *$7fc3", "bsnr,0 *$6fe2", "bsnr,0 $7fff", "bsnr,0 *$0000", "bsnr,0 $1fc2", "bsnr,0 $0ad3", "bsnr,0 $102c", "bsnr,0 *$611e", "bsnr,0 $07b8" } },   /* 78 */
    { 2, 11, { "bsnr,1 $00ff", "bsnr,1 $0125", "bsnr,1 $0020", "bsnr,1 $3fc2", "bsnr,1 $3ff2", "bsnr,1 *$3fff", "bsnr,1 *$4023", "bsnr,1 *$7fc3", "bsnr,1 *$6fe2", "bsnr,1 $7fff", "bsnr,1 *$0000", "bsnr,1 $1fc2", "bsnr,1 $0ad3", "bsnr,1 $102c", "bsnr,1 *$611e", "bsnr,1 $07b8" } },   /* 79 */
    { 2, 11, { "bsnr,2 $00ff", "bsnr,2 $0125", "bsnr,2 $0020", "bsnr,2 $3fc2", "bsnr,2 $3ff2", "bsnr,2 *$3fff", "bsnr,2 *$4023", "bsnr,2 *$7fc3", "bsnr,2 *$6fe2", "bsnr,2 $7fff", "bsnr,2 *$0000", "bsnr,2 $1fc2", "bsnr,2 $0ad3", "bsnr,2 $102c", "bsnr,2 *$611e", "bsnr,2 $07b8" } },   /* 7A */
    { 2, 11, { "bsnr,3 $00ff", "bsnr,3 $0125", "bsnr,3 $0020", "bsnr,3 $3fc2", "bsnr,3 $3ff2", "bsnr,3 *$3fff", "bsnr,3 *$4023", "bsnr,3 *$7fc3", "bsnr,3 *$6fe2", "bsnr,3 $7fff", "bsnr,3 *$0000", "bsnr,3 $1fc2", "bsnr,3 $0ad3", "bsnr,3 $102c", "bsnr,3 *$611e", "bsnr,3 $07b8" } },   /* 7B */
    { 3, 11, { "bsna,0 $5ac3", "bsna,0 $0000", "bsna,0 $20ff", "bsna,0 $4112", "bsna,0 $60a5", "bsna,0 *$0000", "bsna,0 *$217f", "bsna,0 *$4280", "bsna,0 *$6334", "bsna,0 $7fff", "bsna,0 *$7f01", "bsna,0 $409c", "bsna,0 $152a", "bsna,0 $2a55", "bsna,0 *$1c33", "bsna,0 $3fc0" } },   /* 7C */
    { 3, 11, { "bsna,1 $5ac3", "bsna,1 $0000", "bsna,1 $20ff", "bsna,1 $4112", "bsna,1 $60a5", "bsna,1 *$0000", "bsna,1 *$217f", "bsna,1 *$4280", "bsna,1 *$6334", "bsna,1 $7fff", "bsna,1 *$7f01", "bsna,1 $409c", "bsna,1 $152a", "bsna,1 $2a55", "bsna,1 *$1c33", "bsna,1 $3fc0" } },   /* 7D */
    { 3, 11, { "bsna,2 $5ac3", "bsna,2 $0000", "bsna,2 $20ff", "bsna,2 $4112", "bsna,2 $60a5", "bsna,2 *$0000", "bsna,2 *$217f", "bsna,2 *$4280", "bsna,2 *$6334", "bsna,2 $7fff", "bsna,2 *$7f01", "bsna,2 $409c", "bsna,2 $152a", "bsna,2 $2a55", "bsna,2 *$1c33", "bsna,2 $3fc0" } },   /* 7E */
    { 3, 11, { "bsna,3 $5ac3", "bsna,3 $0000", "bsna,3 $20ff", "bsna,3 $4112", "bsna,3 $60a5", "bsna,3 *$0000", "bsna,3 *$217f", "bsna,3 *$4280", "bsna,3 *$6334", "bsna,3 $7fff", "bsna,3 *$7f01", "bsna,3 $409c", "bsna,3 $152a", "bsna,3 $2a55", "bsna,3 *$1c33", "bsna,3 $3fc0" } },   /* 7F */
    { 1, 8, { "addz,0" } },   /* 80 */
    { 1, 8, { "addz,1" } },   /* 81 */
    { 1, 8, { "addz,2" } },   /* 82 */
    { 1, 8, { "addz,3" } },   /* 83 */
    { 2, 8, { "addi,0 $5a", "addi,0 $00", "addi,0 $20", "addi,0 $41", "addi,0 $60", "addi,0 $80", "addi,0 $a1", "addi,0 $c2", "addi,0 $e3", "addi,0 $7f", "addi,0 $ff", "addi,0 $40", "addi,0 $15", "addi,0 $2a", "addi,0 $9c", "addi,0 $3f" } },   /* 84 */
    { 2, 8, { "addi,1 $5a", "addi,1 $00", "addi,1 $20", "addi,1 $41", "addi,1 $60", "addi,1 $80", "addi,1 $a1", "addi,1 $c2", "addi,1 $e3", "addi,1 $7f", "addi,1 $ff", "addi,1 $40", "addi,1 $15", "addi,1 $2a", "addi,1 $9c", "addi,1 $3f" } },   /* 85 */
    { 2, 8, { "addi,2 $5a", "addi,2 $00", "addi,2 $20", "addi,2 $41", "addi,2 $60", "addi,2 $80", "addi,2 $a1", "addi,2 $c2", "addi,2 $e3", "addi,2 $7f", "addi,2 $ff", "addi,2 $40", "addi,2 $15", "addi,2 $2a", "addi,2 $9c", "addi,2 $3f" } },   /* 86 */
    { 2, 8, { "addi,3 $5a", "addi,3 $00", "addi,3 $20", "addi,3 $41", "addi,3 $60", "addi,3 $80", "addi,3 $a1", "addi,3 $c2", "addi,3 $e3", "addi,3 $7f", "addi,3 $ff", "addi,3 $40", "addi,3 $15", "addi,3 $2a", "addi,3 $9c", "addi,3 $3f" } },   /* 87 */
    { 2, 8, { "addr,0 $00ff", "addr,0 $0125", "addr,0 $0020", "addr,0 $3fc2", "addr,0 $3ff2", "addr,0 *$3fff", "addr,0 *$4023", "addr,0 *$7fc3", "addr,0 *$6fe2", "addr,0 $7fff", "addr,0 *$0000", "addr,0 $1fc2", "addr,0 $0ad3", "addr,0 $102c", "addr,0 *$611e", "addr,0 $07b8" } },   /* 88 */
    { 2, 8, { "addr,1 $00ff", "addr,1 $0125", "addr,1 $0020", "addr,1 $3fc2", "addr,1 $3ff2", "addr,1 *$3fff", "addr,1 *$4023", "addr,1 *$7fc3", "addr,1 *$6fe2", "addr,1 $7fff", "addr,1 *$0000", "addr,1 $1fc2", "addr,1 $0ad3", "addr,1 $102c", "addr,1 *$611e", "addr,1 $07b8" } },   /* 89 */
    { 2, 8, { "addr,2 $00ff", "addr,2 $0125", "addr,2 $0020", "addr,2 $3fc2", "addr,2 $3ff2", "addr,2 *$3fff", "addr,2 *$4023", "addr,2 *$7fc3", "addr,2 *$6fe2", "addr,2 $7fff", "addr,2 *$0000", "addr,2 $1fc2", "addr,2 $0ad3", "addr,2 $102c", "addr,2 *$611e", "addr,2 $07b8" } },   /* 8A */
    { 2, 8, { "addr,3 $00ff", "addr,3 $0125", "addr,3 $0020", "addr,3 $3fc2", "addr,3 $3ff2", "addr,3 *$3fff", "addr,3 *$4023", "addr,3 *$7fc3", "addr,3 *$6fe2", "addr,3 $7fff", "addr,3 *$0000", "addr,3 $1fc2", "addr,3 $0ad3", "addr,3 $102c", "addr,3 *$611e", "addr,3 $07b8" } },   /* 8B */
    { 3, 8, { "adda,0 $1ac3,r0-", "adda,0 $0000", "adda,0 $00ff,r0+", "adda,0 $2112,r0-", "adda,0 $20a5,r0", "adda,0 *$2000", "adda,0 *$417f,r0+", "adda,0 *$6280,r0-", "adda,0 *$6334,r0", "adda,0 $7fff,r0", "adda,0 *$1f01,r0", "adda,0 $009c,r0-", "adda,0 $152a", "adda,0 $0a55,r0+", "adda,0 *$7c33", "adda,0 $1fc0,r0+" } },   /* 8C */
    { 3, 8, { "adda,0 $1ac3,r1-", "adda,1 $0000", "adda,0 $00ff,r1+", "adda,0 $2112,r1-", "adda,0 $20a5,r1", "adda,1 *$2000", "adda,0 *$417f,r1+", "adda,0 *$6280,r1-", "adda,0 *$6334,r1", "adda,0 $7fff,r1", "adda,0 *$1f01,r1", "adda,0 $009c,r1-", "adda,1 $152a", "adda,0 $0a55,r1+", "adda,1 *$7c33", "adda,0 $1fc0,r1+" } },   /* 8D */
    { 3, 8, { "adda,0 $1ac3,r2-", "adda,2 $0000", "adda,0 $00ff,r2+", "adda,0 $2112,r2-", "adda,0 $20a5,r2", "adda,2 *$2000", "adda,0 *$417f,r2+", "adda,0 *$6280,r2-", "adda,0 *$6334,r2", "adda,0 $7fff,r2", "adda,0 *$1f01,r2", "adda,0 $009c,r2-", "adda,2 $152a", "adda,0 $0a55,r2+", "adda,2 *$7c33", "adda,0 $1fc0,r2+" } },   /* 8E */
    { 3, 8, { "adda,0 $1ac3,r3-", "adda,3 $0000", "adda,0 $00ff,r3+", "adda,0 $2112,r3-", "adda,0 $20a5,r3", "adda,3 *$2000", "adda,0 *$417f,r3+", "adda,0 *$6280,r3-", "adda,0 *$6334,r3", "adda,0 $7fff,r3", "adda,0 *$1f01,r3", "adda,0 $009c,r3-", "adda,3 $152a", "adda,0 $0a55,r3+", "adda,3 *$7c33", "adda,0 $1fc0,r3+" } },   /* 8F */
    { 1, 8, { "****   $90" } },   /* 90 */
    { 1, 8, { "****   $91" } },   /* 91 */
    { 1, 8, { "lpsu" } },   /* 92 */
    { 1, 8, { "lpsl" } },   /* 93 */
    { 1, 8, { "dar,0" } },   /* 94 */
    { 1, 8, { "dar,1" } },   /* 95 */
    { 1, 8, { "dar,2" } },   /* 96 */
    { 1, 8, { "dar,3" } },   /* 97 */
    { 2, 9, { "bcfr,z $00ff", "bcfr,z $0125", "bcfr,z $0020", "bcfr,z $3fc2", "bcfr,z $3ff2", "bcfr,z *$3fff", "bcfr,z *$4023", "bcfr,z *$7fc3", "bcfr,z *$6fe2", "bcfr,z $7fff", "bcfr,z *$0000", "bcfr,z $1fc2", "bcfr,z $0ad3", "bcfr,z $102c", "bcfr,z *$611e", "bcfr,z $07b8" } },   /* 98 */
    { 2, 9, { "bcfr,p $00ff", "bcfr,p $0125", "bcfr,p $0020", "bcfr,p $3fc2", "bcfr,p $3ff2", "bcfr,p *$3fff", "bcfr,p *$4023", "bcfr,p *$7fc3", "bcfr,p *$6fe2", "bcfr,p $7fff", "bcfr,p *$0000", "bcfr,p $1fc2", "bcfr,p $0ad3", "bcfr,p $102c", "bcfr,p *$611e", "bcfr,p $07b8" } },   /* 99 */
    { 2, 9, { "bcfr,m $00ff", "bcfr,m $0125", "bcfr,m $0020", "bcfr,m $3fc2", "bcfr,m $3ff2", "bcfr,m *$3fff", "bcfr,m *$4023", "bcfr,m *$7fc3", "bcfr,m *$6fe2", "bcfr,m $7fff", "bcfr,m *$0000", "bcfr,m $1fc2", "bcfr,m $0ad3", "bcfr,m $102c", "bcfr,m *$611e", "bcfr,m $07b8" } },   /* 9A */
    { 2, 8, { "zbrr   $1fda", "zbrr   $0000", "zbrr   $0020", "zbrr   $1fc1", "zbrr   $1fe0", "zbrr   *$0000", "zbrr   *$0021", "zbrr   *$1fc2", "zbrr   *$1fe3", "zbrr   $1fff", "zbrr   *$1fff", "zbrr   $1fc0", "zbrr   $0015", "zbrr   $002a", "zbrr   *$001c", "zbrr   $003f" } },   /* 9B */
    { 3, 8, { "bcfa,z $5ac3", "bcfa,z $0000", "bcfa,z $20ff", "bcfa,z $4112", "bcfa,z $60a5", "bcfa,z *$0000", "bcfa,z *$217f", "bcfa,z *$4280", "bcfa,z *$6334", "bcfa,z $7fff", "bcfa,z *$7f01", "bcfa,z $409c", "bcfa,z $152a", "bcfa,z $2a55", "bcfa,z *$1c33", "bcfa,z $3fc0" } },   /* 9C */
    { 3, 8, { "bcfa,p $5ac3", "bcfa,p $0000", "bcfa,p $20ff", "bcfa,p $4112", "bcfa,p $60a5", "bcfa,p *$0000", "bcfa,p *$217f", "bcfa,p *$4280", "bcfa,p *$6334", "bcfa,p $7fff", "bcfa,p *$7f01", "bcfa,p $409c", "bcfa,p $152a", "bcfa,p $2a55", "bcfa,p *$1c33", "bcfa,p $3fc0" } },   /* 9D */
    { 3, 8, { "bcfa,m $5ac3", "bcfa,m $0000", "bcfa,m $20ff", "bcfa,m $4112", "bcfa,m $60a5", "bcfa,m *$0000", "bcfa,m *$217f", "bcfa,m *$4280", "bcfa,m *$6334", "bcfa,m $7fff", "bcfa,m *$7f01", "bcfa,m $409c", "bcfa,m $152a", "bcfa,m $2a55", "bcfa,m *$1c33", "bcfa,m $3fc0" } },   /* 9E */
    { 3, 8, { "bxa    $5ac3", "bxa    $0000", "bxa    $20ff", "bxa    $4112", "bxa    $60a5", "bxa    *$0000", "bxa    *$217f", "bxa    *$4280", "bxa    *$6334", "bxa    $7fff", "bxa    *$7f01", "bxa    $409c", "bxa    $152a", "bxa    $2a55", "bxa    *$1c33", "bxa    $3fc0" } },   /* 9F */
    { 1, 8, { "subz,0" } },   /* A0 */
    { 1, 8, { "subz,1" } },   /* A1 */
    { 1, 8, { "subz,2" } },   /* A2 */
    { 1, 8, { "subz,3" } },   /* A3 */
    { 2, 8, { "subi,0 $5a", "subi,0 $00", "subi,0 $20", "subi,0 $41", "subi,0 $60", "subi,0 $80", "subi,0 $a1", "subi,0 $c2", "subi,0 $e3", "subi,0 $7f", "subi,0 $ff", "subi,0 $40", "subi,0 $15", "subi,0 $2a", "subi,0 $9c", "subi,0 $3f" } },   /* A4 */
    { 2, 8, { "subi,1 $5a", "subi,1 $00", "subi,1 $20", "subi,1 $41", "subi,1 $60", "subi,1 $80", "subi,1 $a1", "subi,1 $c2", "subi,1 $e3", "subi,1 $7f", "subi,1 $ff", "subi,1 $40", "subi,1 $15", "subi,1 $2a", "subi,1 $9c", "subi,1 $3f" } },   /* A5 */
    { 2, 8, { "subi,2 $5a", "subi,2 $00", "subi,2 $20", "subi,2 $41", "subi,2 $60", "subi,2 $80", "subi,2 $a1", "subi,2 $c2", "subi,2 $e3", "subi,2 $7f", "subi,2 $ff", "subi,2 $40", "subi,2 $15", "subi,2 $2a", "subi,2 $9c", "subi,2 $3f" } },   /* A6 */
    { 2, 8, { "subi,3 $5a", "subi,3 $00", "subi,3 $20", "subi,3 $41", "subi,3 $60", "subi,3 $80", "subi,3 $a1", "subi,3 $c2", "subi,3 $e3", "subi,3 $7f", "subi,3 $ff", "subi,3 $40", "subi,3 $15", "subi,3 $2a", "subi,3 $9c", "subi,3 $3f" } },   /* A7 */
    { 2, 8, { "subr,0 $00ff", "subr,0 $0125", "subr,0 $0020", "subr,0 $3fc2", "subr,0 $3ff2", "subr,0 *$3fff", "subr,0 *$4023", "subr,0 *$7fc3", "subr,0 *$6fe2", "subr,0 $7fff", "subr,0 *$0000", "subr,0 $1fc2", "subr,0 $0ad3", "subr,0 $102c", "subr,0 *$611e", "subr,0 $07b8" } },   /* A8 */
    { 2, 8, { "subr,1 $00ff", "subr,1 $0125", "subr,1 $0020", "subr,1 $3fc2", "subr,1 $3ff2", "subr,1 *$3fff", "subr,1 *$4023", "subr,1 *$7fc3", "subr,1 *$6fe2", "subr,1 $7fff", "subr,1 *$0000", "subr,1 $1fc2", "subr,1 $0ad3", "subr,1 $102c", "subr,1 *$611e", "subr,1 $07b8" } },   /* A9 */
    { 2, 8, { "subr,2 $00ff", "subr,2 $0125", "subr,2 $0020", "subr,2 $3fc2", "subr,2 $3ff2", "subr,2 *$3fff", "subr,2 *$4023", "subr,2 *$7fc3", "subr,2 *$6fe2", "subr,2 $7fff", "subr,2 *$0000", "subr,2 $1fc2", "subr,2 $0ad3", "subr,2 $102c", "subr,2 *$611e", "subr,2 $07b8" } },   /* AA */
    { 2, 8, { "subr,3 $00ff", "subr,3 $0125", "subr,3 $0020", "subr,3 $3fc2", "subr,3 $3ff2", "subr,3 *$3fff", "subr,3 *$4023", "subr,3 *$7fc3", "subr,3 *$6fe2", "subr,3 $7fff", "subr,3 *$0000", "subr,3 $1fc2", "subr,3 $0ad3", "subr,3 $102c", "subr,3 *$611e", "subr,3 $07b8" } },   /* AB */
    { 3, 8, { "suba,0 $1ac3,r0-", "suba,0 $0000", "suba,0 $00ff,r0+", "suba,0 $2112,r0-", "suba,0 $20a5,r0", "suba,0 *$2000", "suba,0 *$417f,r0+", "suba,0 *$6280,r0-", "suba,0 *$6334,r0", "suba,0 $7fff,r0", "suba,0 *$1f01,r0", "suba,0 $009c,r0-", "suba,0 $152a", "suba,0 $0a55,r0+", "suba,0 *$7c33", "suba,0 $1fc0,r0+" } },   /* AC */
    { 3, 8, { "suba,0 $1ac3,r1-", "suba,1 $0000", "suba,0 $00ff,r1+", "suba,0 $2112,r1-", "suba,0 $20a5,r1", "suba,1 *$2000", "suba,0 *$417f,r1+", "suba,0 *$6280,r1-", "suba,0 *$6334,r1", "suba,0 $7fff,r1", "suba,0 *$1f01,r1", "suba,0 $009c,r1-", "suba,1 $152a", "suba,0 $0a55,r1+", "suba,1 *$7c33", "suba,0 $1fc0,r1+" } },   /* AD */
    { 3, 8, { "suba,0 $1ac3,r2-", "suba,2 $0000", "suba,0 $00ff,r2+", "suba,0 $2112,r2-", "suba,0 $20a5,r2", "suba,2 *$2000", "suba,0 *$417f,r2+", "suba,0 *$6280,r2-", "suba,0 *$6334,r2", "suba,0 $7fff,r2", "suba,0 *$1f01,r2", "suba,0 $009c,r2-", "suba,2 $152a", "suba,0 $0a55,r2+", "suba,2 *$7c33", "suba,0 $1fc0,r2+" } },   /* AE */
    { 3, 8, { "suba,0 $1ac3,r3-", "suba,3 $0000", "suba,0 $00ff,r3+", "suba,0 $2112,r3-", "suba,0 $20a5,r3", "suba,3 *$2000", "suba,0 *$417f,r3+", "suba,0 *$6280,r3-", "suba,0 *$6334,r3", "suba,0 $7fff,r3", "suba,0 *$1f01,r3", "suba,0 $009c,r3-", "suba,3 $152a", "suba,0 $0a55,r3+", "suba,3 *$7c33", "suba,0 $1fc0,r3+" } },   /* AF */
    { 1, 8, { "wrtc,0" } },   /* B0 */
    { 1, 8, { "wrtc,1" } },   /* B1 */
    { 1, 8, { "wrtc,2" } },   /* B2 */
    { 1, 8, { "wrtc,3" } },   /* B3 */
    { 2, 8, { "tpsu   fo+4+3+sp1", "tpsu   ", "tpsu   ii", "tpsu   fo+sp0", "tpsu   fo+ii", "tpsu   si", "tpsu   si+ii+sp0", "tpsu   si+fo+sp1", "tpsu   si+fo+ii+sp1+sp0", "tpsu   fo+ii+4+3+sp2+sp1+sp0", "tpsu   all", "tpsu   fo", "tpsu   4+sp2+sp0", "tpsu   ii+3+sp1", "tpsu   si+4+3+sp2", "tpsu   ii+4+3+sp2+sp1+sp0" } },   /* B4 */
    { 2, 8, { "tpsl   p+rs+wc+com", "tpsl   ", "tpsl   idc", "tpsl   p+c", "tpsl   p+idc", "tpsl   m", "tpsl   m+idc+c", "tpsl   cc+com", "tpsl   cc+idc+com+c", "tpsl   p+idc+rs+wc+ovf+com+c", "tpsl   all", "tpsl   p", "tpsl   rs+ovf+c", "tpsl   idc+wc+com", "tpsl   m+rs+wc+ovf", "tpsl   idc+rs+wc+ovf+com+c" } },   /* B5 */
    { 1, 8, { "****   $B6" } },   /* B6 */
    { 1, 8, { "****   $B7" } },   /* B7 */
    { 2, 11, { "bsfr,z $00ff", "bsfr,z $0125", "bsfr,z $0020", "bsfr,z $3fc2", "bsfr,z $3ff2", "bsfr,z *$3fff", "bsfr,z *$4023", "bsfr,z *$7fc3", "bsfr,z *$6fe2", "bsfr,z $7fff", "bsfr,z *$0000", "bsfr,z $1fc2", "bsfr,z $0ad3", "bsfr,z $102c", "bsfr,z *$611e", "bsfr,z $07b8" } },   /* B8 */
    { 2, 11, { "bsfr,p $00ff", "bsfr,p $0125", "bsfr,p $0020", "bsfr,p $3fc2", "bsfr,p $3ff2", "bsfr,p *$3fff", "bsfr,p *$4023", "bsfr,p *$7fc3", "bsfr,p *$6fe2", "bsfr,p $7fff", "bsfr,p *$0000", "bsfr,p $1fc2", "bsfr,p $0ad3", "bsfr,p $102c", "bsfr,p *$611e", "bsfr,p $07b8" } },   /* B9 */
    { 2, 11, { "bsfr,m $00ff", "bsfr,m $0125", "bsfr,m $0020", "bsfr,m $3fc2", "bsfr,m $3ff2", "bsfr,m *$3fff", "bsfr,m *$4023", "bsfr,m *$7fc3", "bsfr,m *$6fe2", "bsfr,m $7fff", "bsfr,m *$0000", "bsfr,m $1fc2", "bsfr,m $0ad3", "bsfr,m $102c", "bsfr,m *$611e", "bsfr,m $07b8" } },   /* BA */
    { 2, 10, { "zbsr   $1fda", "zbsr   $0000", "zbsr   $0020", "zbsr   $1fc1", "zbsr   $1fe0", "zbsr   *$0000", "zbsr   *$0021", "zbsr   *$1fc2", "zbsr   *$1fe3", "zbsr   $1fff", "zbsr   *$1fff", "zbsr   $1fc0", "zbsr   $0015", "zbsr   $002a", "zbsr   *$001c", "zbsr   $003f" } },   /* BB */
    { 3, 11, { "bsfa,z $5ac3", "bsfa,z $0000", "bsfa,z $20ff", "bsfa,z $4112", "bsfa,z $60a5", "bsfa,z *$0000", "bsfa,z *$217f", "bsfa,z *$4280", "bsfa,z *$6334", "bsfa,z $7fff", "bsfa,z *$7f01", "bsfa,z $409c", "bsfa,z $152a", "bsfa,z $2a55", "bsfa,z *$1c33", "bsfa,z $3fc0" } },   /* BC */
    { 3, 11, { "bsfa,p $5ac3", "bsfa,p $0000", "bsfa,p $20ff", "bsfa,p $4112", "bsfa,p $60a5", "bsfa,p *$0000", "bsfa,p *$217f", "bsfa,p *$4280", "bsfa,p *$6334", "bsfa,p $7fff", "bsfa,p *$7f01", "bsfa,p $409c", "bsfa,p $152a", "bsfa,p $2a55", "bsfa,p *$1c33", "bsfa,p $3fc0" } },   /* BD */
    { 3, 11, { "bsfa,m $5ac3", "bsfa,m $0000", "bsfa,m $20ff", "bsfa,m $4112", "bsfa,m $60a5", "bsfa,m *$0000", "bsfa,m *$217f", "bsfa,m *$4280", "bsfa,m *$6334", "bsfa,m $7fff", "bsfa,m *$7f01", "bsfa,m $409c", "bsfa,m $152a", "bsfa,m $2a55", "bsfa,m *$1c33", "bsfa,m $3fc0" } },   /* BE */
    { 3, 10, { "bsxa   $5ac3", "bsxa   $0000", "bsxa   $20ff", "bsxa   $4112", "bsxa   $60a5", "bsxa   *$0000", "bsxa   *$217f", "bsxa   *$4280", "bsxa   *$6334", "bsxa   $7fff", "bsxa   *$7f01", "bsxa   $409c", "bsxa   $152a", "bsxa   $2a55", "bsxa   *$1c33", "bsxa   $3fc0" } },   /* BF */
    { 1, 8, { "nop" } },   /* C0 */
    { 1, 8, { "strz,1" } },   /* C1 */
    { 1, 8, { "strz,2" } },   /* C2 */
    { 1, 8, { "strz,3" } },   /* C3 */
    { 1, 8, { "****   $C4" } },   /* C4 */
    { 1, 8, { "****   $C5" } },   /* C5 */
    { 1, 8, { "****   $C6" } },   /* C6 */
    { 1, 8, { "****   $C7" } },   /* C7 */
    { 2, 8, { "strr,0 $00ff", "strr,0 $0125", "strr,0 $0020", "strr,0 $3fc2", "strr,0 $3ff2", "strr,0 *$3fff", "strr,0 *$4023", "strr,0 *$7fc3", "strr,0 *$6fe2", "strr,0 $7fff", "strr,0 *$0000", "strr,0 $1fc2", "strr,0 $0ad3", "strr,0 $102c", "strr,0 *$611e", "strr,0 $07b8" } },   /* C8 */
    { 2, 8, { "strr,1 $00ff", "strr,1 $0125", "strr,1 $0020", "strr,1 $3fc2", "strr,1 $3ff2", "strr,1 *$3fff", "strr,1 *$4023", "strr,1 *$7fc3", "strr,1 *$6fe2", "strr,1 $7fff", "strr,1 *$0000", "strr,1 $1fc2", "strr,1 $0ad3", "strr,1 $102c", "strr,1 *$611e", "strr,1 $07b8" } },   /* C9 */
    { 2, 8, { "strr,2 $00ff", "strr,2 $0125", "strr,2 $0020", "strr,2 $3fc2", "strr,2 $3ff2", "strr,2 *$3fff", "strr,2 *$4023", "strr,2 *$7fc3", "strr,2 *$6fe2", "strr,2 $7fff", "strr,2 *$0000", "strr,2 $1fc2", "strr,2 $0ad3", "strr,2 $102c", "strr,2 *$611e", "strr,2 $07b8" } },   /* CA */
    { 2, 8, { "strr,3 $00ff", "strr,3 $0125", "strr,3 $0020", "strr,3 $3fc2", "strr,3 $3ff2", "strr,3 *$3fff", "strr,3 *$4023", "strr,3 *$7fc3", "strr,3 *$6fe2", "strr,3 $7fff", "strr,3 *$0000", "strr,3 $1fc2", "strr,3 $0ad3", "strr,3 $102c", "strr,3 *$611e", "strr,3 $07b8" } },   /* CB */
    { 3, 8, { "stra,0 $1ac3,r0-", "stra,0 $0000", "stra,0 $00ff,r0+", "stra,0 $2112,r0-", "stra,0 $20a5,r0", "stra,0 *$2000", "stra,0 *$417f,r0+", "stra,0 *$6280,r0-", "stra,0 *$6334,r0", "stra,0 $7fff,r0", "stra,0 *$1f01,r0", "stra,0 $009c,r0-", "stra,0 $152a", "stra,0 $0a55,r0+", "stra,0 *$7c33", "stra,0 $1fc0,r0+" } },   /* CC */
    { 3, 8, { "stra,0 $1ac3,r1-", "stra,1 $0000", "stra,0 $00ff,r1+", "stra,0 $2112,r1-", "stra,0 $20a5,r1", "stra,1 *$2000", "stra,0 *$417f,r1+", "stra,0 *$6280,r1-", "stra,0 *$6334,r1", "stra,0 $7fff,r1", "stra,0 *$1f01,r1", "stra,0 $009c,r1-", "stra,1 $152a", "stra,0 $0a55,r1+", "stra,1 *$7c33", "stra,0 $1fc0,r1+" } },   /* CD */
    { 3, 8, { "stra,0 $1ac3,r2-", "stra,2 $0000", "stra,0 $00ff,r2+", "stra,0 $2112,r2-", "stra,0 $20a5,r2", "stra,2 *$2000", "stra,0 *$417f,r2+", "stra,0 *$6280,r2-", "stra,0 *$6334,r2", "stra,0 $7fff,r2", "stra,0 *$1f01,r2", "stra,0 $009c,r2-", "stra,2 $152a", "stra,0 $0a55,r2+", "stra,2 *$7c33", "stra,0 $1fc0,r2+" } },   /* CE */
    { 3, 8, { "stra,0 $1ac3,r3-", "stra,3 $0000", "stra,0 $00ff,r3+", "stra,0 $2112,r3-", "stra,0 $20a5,r3", "stra,3 *$2000", "stra,0 *$417f,r3+", "stra,0 *$6280,r3-", "stra,0 *$6334,r3", "stra,0 $7fff,r3", "stra,0 *$1f01,r3", "stra,0 $009c,r3-", "stra,3 $152a", "stra,0 $0a55,r3+", "stra,3 *$7c33", "stra,0 $1fc0,r3+" } },   /* CF */
    { 1, 8, { "rrl,0" } },   /* D0 */
    { 1, 8, { "rrl,1" } },   /* D1 */
    { 1, 8, { "rrl,2" } },   /* D2 */
    { 1, 8, { "rrl,3" } },   /* D3 */
    { 2, 8, { "wrte,0 $5a", "wrte,0 $00", "wrte,0 $20", "wrte,0 $41", "wrte,0 $60", "wrte,0 $80", "wrte,0 $a1", "wrte,0 $c2", "wrte,0 $e3", "wrte,0 $7f", "wrte,0 $ff", "wrte,0 $40", "wrte,0 $15", "wrte,0 $2a", "wrte,0 $9c", "wrte,0 $3f" } },   /* D4 */
    { 2, 8, { "wrte,1 $5a", "wrte,1 $00", "wrte,1 $20", "wrte,1 $41", "wrte,1 $60", "wrte,1 $80", "wrte,1 $a1", "wrte,1 $c2", "wrte,1 $e3", "wrte,1 $7f", "wrte,1 $ff", "wrte,1 $40", "wrte,1 $15", "wrte,1 $2a", "wrte,1 $9c", "wrte,1 $3f" } },   /* D5 */
    { 2, 8, { "wrte,2 $5a", "wrte,2 $00", "wrte,2 $20", "wrte,2 $41", "wrte,2 $60", "wrte,2 $80", "wrte,2 $a1", "wrte,2 $c2", "wrte,2 $e3", "wrte,2 $7f", "wrte,2 $ff", "wrte,2 $40", "wrte,2 $15", "wrte,2 $2a", "wrte,2 $9c", "wrte,2 $3f" } },   /* D6 */
    { 2, 8, { "wrte,3 $5a", "wrte,3 $00", "wrte,3 $20", "wrte,3 $41", "wrte,3 $60", "wrte,3 $80", "wrte,3 $a1", "wrte,3 $c2", "wrte,3 $e3", "wrte,3 $7f", "wrte,3 $ff", "wrte,3 $40", "wrte,3 $15", "wrte,3 $2a", "wrte,3 $9c", "wrte,3 $3f" } },   /* D7 */
    { 2, 9, { "birr,0 $00ff", "birr,0 $0125", "birr,0 $0020", "birr,0 $3fc2", "birr,0 $3ff2", "birr,0 *$3fff", "birr,0 *$4023", "birr,0 *$7fc3", "birr,0 *$6fe2", "birr,0 $7fff", "birr,0 *$0000", "birr,0 $1fc2", "birr,0 $0ad3", "birr,0 $102c", "birr,0 *$611e", "birr,0 $07b8" } },   /* D8 */
    { 2, 9, { "birr,1 $00ff", "birr,1 $0125", "birr,1 $0020", "birr,1 $3fc2", "birr,1 $3ff2", "birr,1 *$3fff", "birr,1 *$4023", "birr,1 *$7fc3", "birr,1 *$6fe2", "birr,1 $7fff", "birr,1 *$0000", "birr,1 $1fc2", "birr,1 $0ad3", "birr,1 $102c", "birr,1 *$611e", "birr,1 $07b8" } },   /* D9 */
    { 2, 9, { "birr,2 $00ff", "birr,2 $0125", "birr,2 $0020", "birr,2 $3fc2", "birr,2 $3ff2", "birr,2 *$3fff", "birr,2 *$4023", "birr,2 *$7fc3", "birr,2 *$6fe2", "birr,2 $7fff", "birr,2 *$0000", "birr,2 $1fc2", "birr,2 $0ad3", "birr,2 $102c", "birr,2 *$611e", "birr,2 $07b8" } },   /* DA */
    { 2, 9, { "birr,3 $00ff", "birr,3 $0125", "birr,3 $0020", "birr,3 $3fc2", "birr,3 $3ff2", "birr,3 *$3fff", "birr,3 *$4023", "birr,3 *$7fc3", "birr,3 *$6fe2", "birr,3 $7fff", "birr,3 *$0000", "birr,3 $1fc2", "birr,3 $0ad3", "birr,3 $102c", "birr,3 *$611e", "birr,3 $07b8" } },   /* DB */
    { 3, 9, { "bira,0 $5ac3", "bira,0 $0000", "bira,0 $20ff", "bira,0 $4112", "bira,0 $60a5", "bira,0 *$0000", "bira,0 *$217f", "bira,0 *$4280", "bira,0 *$6334", "bira,0 $7fff", "bira,0 *$7f01", "bira,0 $409c", "bira,0 $152a", "bira,0 $2a55", "bira,0 *$1c33", "bira,0 $3fc0" } },   /* DC */
    { 3, 9, { "bira,1 $5ac3", "bira,1 $0000", "bira,1 $20ff", "bira,1 $4112", "bira,1 $60a5", "bira,1 *$0000", "bira,1 *$217f", "bira,1 *$4280", "bira,1 *$6334", "bira,1 $7fff", "bira,1 *$7f01", "bira,1 $409c", "bira,1 $152a", "bira,1 $2a55", "bira,1 *$1c33", "bira,1 $3fc0" } },   /* DD */
    { 3, 9, { "bira,2 $5ac3", "bira,2 $0000", "bira,2 $20ff", "bira,2 $4112", "bira,2 $60a5", "bira,2 *$0000", "bira,2 *$217f", "bira,2 *$4280", "bira,2 *$6334", "bira,2 $7fff", "bira,2 *$7f01", "bira,2 $409c", "bira,2 $152a", "bira,2 $2a55", "bira,2 *$1c33", "bira,2 $3fc0" } },   /* DE */
    { 3, 9, { "bira,3 $5ac3", "bira,3 $0000", "bira,3 $20ff", "bira,3 $4112", "bira,3 $60a5", "bira,3 *$0000", "bira,3 *$217f", "bira,3 *$4280", "bira,3 *$6334", "bira,3 $7fff", "bira,3 *$7f01", "bira,3 $409c", "bira,3 $152a", "bira,3 $2a55", "bira,3 *$1c33", "bira,3 $3fc0" } },   /* DF */
    { 1, 8, { "comz,0" } },   /* E0 */
    { 1, 8, { "comz,1" } },   /* E1 */
    { 1, 8, { "comz,2" } },   /* E2 */
    { 1, 8, { "comz,3" } },   /* E3 */
    { 2, 8, { "comi,0 $5a", "comi,0 $00", "comi,0 $20", "comi,0 $41", "comi,0 $60", "comi,0 $80", "comi,0 $a1", "comi,0 $c2", "comi,0 $e3", "comi,0 $7f", "comi,0 $ff", "comi,0 $40", "comi,0 $15", "comi,0 $2a", "comi,0 $9c", "comi,0 $3f" } },   /* E4 */
    { 2, 8, { "comi,1 $5a", "comi,1 $00", "comi,1 $20", "comi,1 $41", "comi,1 $60", "comi,1 $80", "comi,1 $a1", "comi,1 $c2", "comi,1 $e3", "comi,1 $7f", "comi,1 $ff", "comi,1 $40", "comi,1 $15", "comi,1 $2a", "comi,1 $9c", "comi,1 $3f" } },   /* E5 */
    { 2, 8, { "comi,2 $5a", "comi,2 $00", "comi,2 $20", "comi,2 $41", "comi,2 $60", "comi,2 $80", "comi,2 $a1", "comi,2 $c2", "comi,2 $e3", "comi,2 $7f", "comi,2 $ff", "comi,2 $40", "comi,2 $15", "comi,2 $2a", "comi,2 $9c", "comi,2 $3f" } },   /* E6 */
    { 2, 8, { "comi,3 $5a", "comi,3 $00", "comi,3 $20", "comi,3 $41", "comi,3 $60", "comi,3 $80", "comi,3 $a1", "comi,3 $c2", "comi,3 $e3", "comi,3 $7f", "comi,3 $ff", "comi,3 $40", "comi,3 $15", "comi,3 $2a", "comi,3 $9c", "comi,3 $3f" } },   /* E7 */
    { 2, 8, { "comr,0 $00ff", "comr,0 $0125", "comr,0 $0020", "comr,0 $3fc2", "comr,0 $3ff2", "comr,0 *$3fff", "comr,0 *$4023", "comr,0 *$7fc3", "comr,0 *$6fe2", "comr,0 $7fff", "comr,0 *$0000", "comr,0 $1fc2", "comr,0 $0ad3", "comr,0 $102c", "comr,0 *$611e", "comr,0 $07b8" } },   /* E8 */
    { 2, 8, { "comr,1 $00ff", "comr,1 $0125", "comr,1 $0020", "comr,1 $3fc2", "comr,1 $3ff2", "comr,1 *$3fff", "comr,1 *$4023", "comr,1 *$7fc3", "comr,1 *$6fe2", "comr,1 $7fff", "comr,1 *$0000", "comr,1 $1fc2", "comr,1 $0ad3", "comr,1 $102c", "comr,1 *$611e", "comr,1 $07b8" } },   /* E9 */
    { 2, 8, { "comr,2 $00ff", "comr,2 $0125", "comr,2 $0020", "comr,2 $3fc2", "comr,2 $3ff2", "comr,2 *$3fff", "comr,2 *$4023", "comr,2 *$7fc3", "comr,2 *$6fe2", "comr,2 $7fff", "comr,2 *$0000", "comr,2 $1fc2", "comr,2 $0ad3", "comr,2 $102c", "comr,2 *$611e", "comr,2 $07b8" } },   /* EA */
    { 2, 8, { "comr,3 $00ff", "comr,3 $0125", "comr,3 $0020", "comr,3 $3fc2", "comr,3 $3ff2", "comr,3 *$3fff", "comr,3 *$4023", "comr,3 *$7fc3", "comr,3 *$6fe2", "comr,3 $7fff", "comr,3 *$0000", "comr,3 $1fc2", "comr,3 $0ad3", "comr,3 $102c", "comr,3 *$611e", "comr,3 $07b8" } },   /* EB */
    { 3, 8, { "coma,0 $1ac3,r0-", "coma,0 $0000", "coma,0 $00ff,r0+", "coma,0 $2112,r0-", "coma,0 $20a5,r0", "coma,0 *$2000", "coma,0 *$417f,r0+", "coma,0 *$6280,r0-", "coma,0 *$6334,r0", "coma,0 $7fff,r0", "coma,0 *$1f01,r0", "coma,0 $009c,r0-", "coma,0 $152a", "coma,0 $0a55,r0+", "coma,0 *$7c33", "coma,0 $1fc0,r0+" } },   /* EC */
    { 3, 8, { "coma,0 $1ac3,r1-", "coma,1 $0000", "coma,0 $00ff,r1+", "coma,0 $2112,r1-", "coma,0 $20a5,r1", "coma,1 *$2000", "coma,0 *$417f,r1+", "coma,0 *$6280,r1-", "coma,0 *$6334,r1", "coma,0 $7fff,r1", "coma,0 *$1f01,r1", "coma,0 $009c,r1-", "coma,1 $152a", "coma,0 $0a55,r1+", "coma,1 *$7c33", "coma,0 $1fc0,r1+" } },   /* ED */
    { 3, 8, { "coma,0 $1ac3,r2-", "coma,2 $0000", "coma,0 $00ff,r2+", "coma,0 $2112,r2-", "coma,0 $20a5,r2", "coma,2 *$2000", "coma,0 *$417f,r2+", "coma,0 *$6280,r2-", "coma,0 *$6334,r2", "coma,0 $7fff,r2", "coma,0 *$1f01,r2", "coma,0 $009c,r2-", "coma,2 $152a", "coma,0 $0a55,r2+", "coma,2 *$7c33", "coma,0 $1fc0,r2+" } },   /* EE */
    { 3, 8, { "coma,0 $1ac3,r3-", "coma,3 $0000", "coma,0 $00ff,r3+", "coma,0 $2112,r3-", "coma,0 $20a5,r3", "coma,3 *$2000", "coma,0 *$417f,r3+", "coma,0 *$6280,r3-", "coma,0 *$6334,r3", "coma,0 $7fff,r3", "coma,0 *$1f01,r3", "coma,0 $009c,r3-", "coma,3 $152a", "coma,0 $0a55,r3+", "coma,3 *$7c33", "coma,0 $1fc0,r3+" } },   /* EF */
    { 1, 8, { "wrtd,0" } },   /* F0 */
    { 1, 8, { "wrtd,1" } },   /* F1 */
    { 1, 8, { "wrtd,2" } },   /* F2 */
    { 1, 8, { "wrtd,3" } },   /* F3 */
    { 2, 8, { "tmi,0  $5a", "tmi,0  $00", "tmi,0  $20", "tmi,0  $41", "tmi,0  $60", "tmi,0  $80", "tmi,0  $a1", "tmi,0  $c2", "tmi,0  $e3", "tmi,0  $7f", "tmi,0  $ff", "tmi,0  $40", "tmi,0  $15", "tmi,0  $2a", "tmi,0  $9c", "tmi,0  $3f" } },   /* F4 */
    { 2, 8, { "tmi,1  $5a", "tmi,1  $00", "tmi,1  $20", "tmi,1  $41", "tmi,1  $60", "tmi,1  $80", "tmi,1  $a1", "tmi,1  $c2", "tmi,1  $e3", "tmi,1  $7f", "tmi,1  $ff", "tmi,1  $40", "tmi,1  $15", "tmi,1  $2a", "tmi,1  $9c", "tmi,1  $3f" } },   /* F5 */
    { 2, 8, { "tmi,2  $5a", "tmi,2  $00", "tmi,2  $20", "tmi,2  $41", "tmi,2  $60", "tmi,2  $80", "tmi,2  $a1", "tmi,2  $c2", "tmi,2  $e3", "tmi,2  $7f", "tmi,2  $ff", "tmi,2  $40", "tmi,2  $15", "tmi,2  $2a", "tmi,2  $9c", "tmi,2  $3f" } },   /* F6 */
    { 2, 8, { "tmi,3  $5a", "tmi,3  $00", "tmi,3  $20", "tmi,3  $41", "tmi,3  $60", "tmi,3  $80", "tmi,3  $a1", "tmi,3  $c2", "tmi,3  $e3", "tmi,3  $7f", "tmi,3  $ff", "tmi,3  $40", "tmi,3  $15", "tmi,3  $2a", "tmi,3  $9c", "tmi,3  $3f" } },   /* F7 */
    { 2, 9, { "bdrr,0 $00ff", "bdrr,0 $0125", "bdrr,0 $0020", "bdrr,0 $3fc2", "bdrr,0 $3ff2", "bdrr,0 *$3fff", "bdrr,0 *$4023", "bdrr,0 *$7fc3", "bdrr,0 *$6fe2", "bdrr,0 $7fff", "bdrr,0 *$0000", "bdrr,0 $1fc2", "bdrr,0 $0ad3", "bdrr,0 $102c", "bdrr,0 *$611e", "bdrr,0 $07b8" } },   /* F8 */
    { 2, 9, { "bdrr,1 $00ff", "bdrr,1 $0125", "bdrr,1 $0020", "bdrr,1 $3fc2", "bdrr,1 $3ff2", "bdrr,1 *$3fff", "bdrr,1 *$4023", "bdrr,1 *$7fc3", "bdrr,1 *$6fe2", "bdrr,1 $7fff", "bdrr,1 *$0000", "bdrr,1 $1fc2", "bdrr,1 $0ad3", "bdrr,1 $102c", "bdrr,1 *$611e", "bdrr,1 $07b8" } },   /* F9 */
    { 2, 9, { "bdrr,2 $00ff", "bdrr,2 $0125", "bdrr,2 $0020", "bdrr,2 $3fc2", "bdrr,2 $3ff2", "bdrr,2 *$3fff", "bdrr,2 *$4023", "bdrr,2 *$7fc3", "bdrr,2 *$6fe2", "bdrr,2 $7fff", "bdrr,2 *$0000", "bdrr,2 $1fc2", "bdrr,2 $0ad3", "bdrr,2 $102c", "bdrr,2 *$611e", "bdrr,2 $07b8" } },   /* FA */
    { 2, 9, { "bdrr,3 $00ff", "bdrr,3 $0125", "bdrr,3 $0020", "bdrr,3 $3fc2", "bdrr,3 $3ff2", "bdrr,3 *$3fff", "bdrr,3 *$4023", "bdrr,3 *$7fc3", "bdrr,3 *$6fe2", "bdrr,3 $7fff", "bdrr,3 *$0000", "bdrr,3 $1fc2", "bdrr,3 $0ad3", "bdrr,3 $102c", "bdrr,3 *$611e", "bdrr,3 $07b8" } },   /* FB */
    { 3, 9, { "bdra,0 $5ac3", "bdra,0 $0000", "bdra,0 $20ff", "bdra,0 $4112", "bdra,0 $60a5", "bdra,0 *$0000", "bdra,0 *$217f", "bdra,0 *$4280", "bdra,0 *$6334", "bdra,0 $7fff", "bdra,0 *$7f01", "bdra,0 $409c", "bdra,0 $152a", "bdra,0 $2a55", "bdra,0 *$1c33", "bdra,0 $3fc0" } },   /* FC */
    { 3, 9, { "bdra,1 $5ac3", "bdra,1 $0000", "bdra,1 $20ff", "bdra,1 $4112", "bdra,1 $60a5", "bdra,1 *$0000", "bdra,1 *$217f", "bdra,1 *$4280", "bdra,1 *$6334", "bdra,1 $7fff", "bdra,1 *$7f01", "bdra,1 $409c", "bdra,1 $152a", "bdra,1 $2a55", "bdra,1 *$1c33", "bdra,1 $3fc0" } },   /* FD */
    { 3, 9, { "bdra,2 $5ac3", "bdra,2 $0000", "bdra,2 $20ff", "bdra,2 $4112", "bdra,2 $60a5", "bdra,2 *$0000", "bdra,2 *$217f", "bdra,2 *$4280", "bdra,2 *$6334", "bdra,2 $7fff", "bdra,2 *$7f01", "bdra,2 $409c", "bdra,2 $152a", "bdra,2 $2a55", "bdra,2 *$1c33", "bdra,2 $3fc0" } },   /* FE */
    { 3, 9, { "bdra,3 $5ac3", "bdra,3 $0000", "bdra,3 $20ff", "bdra,3 $4112", "bdra,3 $60a5", "bdra,3 *$0000", "bdra,3 *$217f", "bdra,3 *$4280", "bdra,3 *$6334", "bdra,3 $7fff", "bdra,3 *$7f01", "bdra,3 $409c", "bdra,3 $152a", "bdra,3 $2a55", "bdra,3 *$1c33", "bdra,3 $3fc0" } },   /* FF */
};
