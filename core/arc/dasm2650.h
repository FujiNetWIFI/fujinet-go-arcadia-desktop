/* dasm2650.h -- the Signetics 2650 disassembler.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * (transposed from MAME src/devices/cpu/s2650/2650dasm.cpp; copyright-holders
 * Juergen Buchmueller -- see COMPLIANCE.md)
 *
 * Text, spacing, case and operand format as MAME's s2650_disassembler in its
 * default (Signetics mnemonic, not Z80 mnemonic) mode, so a listing here and
 * MAME's debugger read alike. core/tests/dasm2650_test.c holds the two to
 * each other for all 256 opcodes.
 */

#ifndef ARC_DASM2650_H
#define ARC_DASM2650_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The flag bits MAME's util::disasm_interface returns beside the length
 * (same values), delivered in *flags. */
#define DASM2650_SUPPORTED   0x80000000u  /* the flags below are meaningful */
#define DASM2650_STEP_OUT    0x40000000u  /* RETC, RETE: the end of a step out */
#define DASM2650_STEP_OVER   0x20000000u  /* BSTR/BSTA/BSNR/BSNA/BSFR/BSFA/ZBSR/BSXA */
#define DASM2650_STEP_COND   0x10000000u  /* conditional: the branch/return may not be taken */

/* Disassemble the instruction at `pc` (15-bit) into out (at most outsz bytes,
 * always terminated, NULL/0 only measures). `opcodes` holds the opcode and the
 * next two bytes: opcodes[1] is the byte at pc + 1, opcodes[2] the byte MAME
 * reads for the low half of an absolute address, dasm2650_operand2_addr(pc).
 * Returns the length in bytes (1-3; an illegal opcode prints "****   $xx",
 * length 1). Relative operands print as MAME does: the page comes from the
 * address of the operand byte, the offset wraps inside the 8K page. */
int dasm2650(uint16_t pc, const uint8_t *opcodes, char *out, size_t outsz);

/* The same, with MAME's flags (DASM2650_*) in *flags (may be NULL). */
int dasm2650_flags(uint16_t pc, const uint8_t *opcodes, char *out, size_t outsz, uint32_t *flags);

/* The length of the instruction that starts with this opcode: 1, 2 or 3. */
int dasm2650_length(uint8_t opcode);

/* MAME's flags for an opcode alone (DASM2650_SUPPORTED | STEP_*), for the
 * debugger's step over / step out. */
uint32_t dasm2650_op_flags(uint8_t opcode);

/* The address of the second operand byte of a 3-byte instruction at `pc`
 * (MAME wraps it inside the 8K page of the first operand byte). */
uint16_t dasm2650_operand2_addr(uint16_t pc);

/* Where a branch at `pc` goes, or -1 for any other opcode: absolute branches
 * (BCTA, BCFA, BRNA, BIRA, BDRA, BSTA, BSFA, BSNA) the 15-bit address with
 * the indirect bit dropped; BXA/BSXA, whose target needs R3, and branches
 * through a pointer ("*") return -1. Relative branches (BCTR ... ) and ZBRR/
 * ZBSR give the direct target (the pointer's address when indirect is -1). */
int dasm2650_target(uint16_t pc, const uint8_t *opcodes);

#ifdef __cplusplus
}
#endif

#endif /* ARC_DASM2650_H */
