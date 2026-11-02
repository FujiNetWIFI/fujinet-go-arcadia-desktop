// s2650_dasm_golden.cpp -- MAME's own 2650 disassembler, driven over every
// opcode, to produce core/tests/dasm2650_golden.h (see gen_dasm2650_golden.sh).
//
// Compiles MAME's src/devices/cpu/s2650/2650dasm.cpp standalone: it needs only
// util::disasm_interface, util::stream_format and util::sext, so a short
// `emu.h` shim stands in for the whole emulator. Signetics-mnemonic mode.
//
// SPDX-License-Identifier: BSD-3-Clause
// (drives MAME's 2650dasm.cpp; copyright-holders Juergen Buchmueller -- see COMPLIANCE.md)

#include "emu.h"
#include "2650dasm.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace {

struct variant { unsigned pc, b1, b2; };

// pc, first and second operand byte. The second byte is read by MAME from
// (p & 0x6000) + ((p + 1) & 0x1fff) with p the first operand's address; the
// golden stores it as opcodes[2]. b1 walks the eight ABS modes (bits 7-5), the
// relative sign/indirect bits, the PSL/PSU masks ("all", empty, singles).
const variant variants[] = {
	{ 0x0123, 0x5a, 0xc3 }, { 0x0123, 0x00, 0x00 }, { 0x1ffe, 0x20, 0xff }, { 0x1fff, 0x41, 0x12 },
	{ 0x2010, 0x60, 0xa5 }, { 0x3ffd, 0x80, 0x00 }, { 0x4000, 0xa1, 0x7f }, { 0x5fff, 0xc2, 0x80 },
	{ 0x6ffd, 0xe3, 0x34 }, { 0x7ffe, 0x7f, 0xff }, { 0x7fff, 0xff, 0x01 }, { 0x0000, 0x40, 0x9c },
	{ 0x0abc, 0x15, 0x2a }, { 0x1000, 0x2a, 0x55 }, { 0x6100, 0x9c, 0x33 }, { 0x0777, 0x3f, 0xc0 },
};
const int NV = sizeof variants / sizeof variants[0];

struct bytes : util::disasm_interface::data_buffer
{
	util::disasm_interface::u8 b[0x8000] = {};
	util::disasm_interface::u8 r8(util::disasm_interface::offs_t pc) const override { return b[pc & 0x7fff]; }
	util::disasm_interface::u16 r16(util::disasm_interface::offs_t) const override { return 0; }
	util::disasm_interface::u32 r32(util::disasm_interface::offs_t) const override { return 0; }
	util::disasm_interface::u64 r64(util::disasm_interface::offs_t) const override { return 0; }
};

struct cfg : s2650_disassembler::config
{
	bool get_z80_mnemonics_mode() const override { return false; }
};

} // namespace

int main()
{
	cfg c;
	s2650_disassembler dasm(&c);
	static bytes buf;

	std::printf("/* dasm2650_golden.h -- MAME's own disassembly of every 2650 opcode, from\n"
	            " * src/devices/cpu/s2650/2650dasm.cpp compiled standalone (tools/ab/\n"
	            " * s2650_dasm_golden.cpp, run by tools/ab/gen_dasm2650_golden.sh). GENERATED;\n"
	            " * dasm2650_test.c holds dasm2650.c to it.\n"
	            " *\n"
	            " * Each opcode is disassembled at the NV pc/operand triples of\n"
	            " * dasm2650_variants (b2 is the byte MAME reads for an absolute address's\n"
	            " * low half); a one-byte opcode has one text, the others NV.\n"
	            " * flags is MAME's result >> 28: 8 = SUPPORTED, 4 = STEP_OUT, 2 = STEP_OVER,\n"
	            " * 1 = STEP_COND. */\n");
	std::printf("#define DASM2650_NV %d\n", NV);
	std::printf("static const struct { uint16_t pc; uint8_t b1, b2; } dasm2650_variants[DASM2650_NV] = {\n");
	for (const variant &v : variants)
		std::printf("    { 0x%04X, 0x%02X, 0x%02X },\n", v.pc, v.b1, v.b2);
	std::printf("};\n");
	std::printf("static const struct { uint8_t len; uint8_t flags; const char *text[DASM2650_NV]; } dasm2650_golden[256] = {\n");

	for (unsigned op = 0; op < 256; op++)
	{
		std::string text[NV];
		unsigned len = 0, flags = 0;
		for (int vi = 0; vi < NV; vi++)
		{
			const variant &v = variants[vi];
			unsigned p1 = v.pc + 1;                              // first operand byte
			unsigned p2 = (p1 & 0x6000) + ((p1 + 1) & 0x1fff);   // second, as MAME reads it
			for (unsigned i = 0; i < 0x8000; i++) buf.b[i] = 0;
			buf.b[v.pc & 0x7fff] = op;
			buf.b[p2 & 0x7fff] = v.b2;
			buf.b[p1 & 0x7fff] = v.b1;      // wins if p1 == p2 (cannot happen) 
			std::ostringstream os;
			util::disasm_interface::offs_t r = dasm.disassemble(os, v.pc, buf, buf);
			len = r & util::disasm_interface::LENGTHMASK;
			flags = (r & ~util::disasm_interface::LENGTHMASK) >> 28;
			text[vi] = os.str();
		}
		std::printf("    { %u, %u, { \"%s\"", len, flags, text[0].c_str());
		if (len > 1)
			for (int vi = 1; vi < NV; vi++)
				std::printf(", \"%s\"", text[vi].c_str());
		std::printf(" } },   /* %02X */\n", op);
	}
	std::printf("};\n");
	return 0;
}
