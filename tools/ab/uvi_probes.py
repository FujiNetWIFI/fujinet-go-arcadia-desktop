#!/usr/bin/env python3
"""uvi_probes.py -- cartridge images that exercise the UVI beyond what the games do.

    uvi_probes.py OUTDIR            write the probes as OUTDIR/uvi_<name>.bin

No assembler: a probe is built with the Asm of s2650_probes.py. Each is an 8K
cartridge (blocks at $0000 and $2000) that rewrites the UVI's registers, the
character RAM and the user defined characters in loops with no relation to the
frame, so every frame samples the writes at a different line and a different
CPU cycle. cart_ab.py then holds the whole picture of every frame to MAME's:
the line timer, the row/sprite drawing order, the collision bits (the probes
copy them onto the screen), the pots and the keypads (likewise).

    cart_ab.py --frames 400 --play OUTDIR/uvi_*.bin

SPDX-License-Identifier: GPL-3.0-or-later
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import s2650_probes as P  # noqa: E402

CPSL, PPSL = 0x75, 0x77


def image(a):
    a.resolve()
    rom = bytearray([0xC0]) * 8192
    for addr, b in a.mem.items():
        assert addr < 0x1000 or 0x2000 <= addr < 0x3000, hex(addr)
        rom[addr if addr < 0x1000 else addr - 0x1000] = b
    return bytes(rom)


def sta(a, addr):
    a.mem_a(0xCC, addr, 0)


def lda(a, addr):
    a.mem_a(0x0C, addr, 0)


def stx(a, addr):          # STRA,R0 addr,R1 (indexed by R1)
    a.mem_a(0xCD, addr, 3)


def ldx(a, addr):
    a.mem_a(0x0D, addr, 3)


def setreg(a, addr, v):
    a.lodi(0, v)
    sta(a, addr)


def lfsr(a):
    """R3 = next of a byte xorshift; clobbers R0"""
    a.lodz(3)
    a.emit(0xD0)                       # RRL,R0
    a.emit(0x24, 0x1D)                 # EORI,R0 $1D
    a.emit(0x23)                       # EORZ R3
    a.strz(3)
    a.emit(0x84 + 0, 0x35)             # ADDI,R0 $35 (WC clear: no carry in)
    a.strz(3)


def probe_chaos(seed):
    """random-ish writes to every UVI register, unsynchronised"""
    a = P.Asm()
    a.org(0)
    a.bcta(3, "start")
    a.org(0x20)
    a.label("start")
    a.cpsl(0x09)                       # WC and C clear
    a.lodi(3, seed)
    setreg(a, 0x18FC, 0xF0)
    setreg(a, 0x19F9, 0x80 | 0x1B)
    a.label("loop")
    lfsr(a)
    # character RAM, upper and lower
    a.lodz(3); a.emit(0x44, 0x7F); a.strz(1)       # R1 = R3 & $7F
    lfsr(a)
    stx(a, 0x1800)
    lfsr(a); a.lodz(3); a.emit(0x44, 0x7F); a.strz(1); lfsr(a)
    stx(a, 0x1A00)
    # user defined characters / object images
    lfsr(a); a.lodz(3); a.emit(0x44, 0x3F); a.strz(1); lfsr(a)
    stx(a, 0x1980)
    # the palette registers and collision registers
    lfsr(a); a.lodz(3); a.emit(0x44, 0x03); a.strz(1); lfsr(a)
    stx(a, 0x19F8)
    # object positions
    lfsr(a); a.lodz(3); a.emit(0x44, 0x07); a.strz(1); lfsr(a)
    stx(a, 0x18F0)
    # now and then: first character line, scroll
    lfsr(a); a.lodz(3); a.emit(0x44, 0x1F)
    a.emit(0x64, 0xE0)
    sta(a, 0x18FC)
    lfsr(a); a.lodz(3); a.emit(0x44, 0xE0)
    sta(a, 0x18FE)
    a.bcta(3, "loop")
    return image(a)


def probe_frames(seed):
    """vblank-synchronised animation: scroll, ypos, refresh mode, graphics mode,
    sizes, colours, objects and a redrawn screen, from a frame counter"""
    a = P.Asm()
    a.org(0)
    a.bcta(3, "start")
    a.org(0x20)
    a.label("start")
    a.cpsl(0x09)
    a.lodi(3, seed)
    a.label("frame")
    # wait for the start of the vertical blank
    a.label("w1"); a.tpsu(0x80); a.bctr(0, "w1")      # still in the last one
    a.label("w2"); a.tpsu(0x80); a.bcfr(0, "w2")
    a.lodi(0, 1); a.emit(0x83)                         # ADDZ R3
    a.strz(3)                                          # R3 = frame counter
    a.lodz(3); a.emit(0x44, 0x07); a.emit(0xD0); a.emit(0xD0); a.emit(0xD0); a.emit(0xD0); a.emit(0xD0)
    sta(a, 0x18FE)                                     # scroll 0-7 in bits 5-7
    a.lodz(3); a.emit(0x50 + 0); a.emit(0x44, 0x1F); a.emit(0x64, 0xE0)
    sta(a, 0x18FC)                                     # ypos moves
    a.lodz(3); a.emit(0x44, 0xC0); a.emit(0x64, 0x08)  # refresh mode / graphics bits
    sta(a, 0x19F8)
    a.lodz(3); sta(a, 0x19F9)                          # colours, mux, size
    a.lodz(3); a.emit(0x84, 0x55); sta(a, 0x19FA)
    a.lodz(3); a.emit(0x84, 0xAA); sta(a, 0x19FB)
    for i, m in enumerate((3, 5, 7, 11, 13, 17, 19, 23)):
        a.lodz(3)
        for _ in range(m % 3):
            a.emit(0x84, m)
        sta(a, 0x18F0 + i)
    # redraw both screens and the UDCs from the counter
    a.lodi(1, 0xCF)
    a.label("f1")
    a.lodz(1); a.emit(0x83); a.emit(0x24, 0x15)
    stx(a, 0x1800)
    a.emit(0x20 + 0)
    a.lodz(1); a.emit(0x23); a.emit(0x84, 0x3)
    stx(a, 0x1A00)
    a.bdrr(1, "f1")
    a.lodi(1, 0x3F)
    a.label("f2")
    a.lodz(1); a.emit(0x83); a.emit(0x24, 0x5A)
    stx(a, 0x1980)
    a.bdrr(1, "f2")
    # the collision, pot and key registers onto the screen where they can be seen
    lda(a, 0x19FC); sta(a, 0x1800 + 0xC0)
    lda(a, 0x19FD); sta(a, 0x1800 + 0xC1)
    lda(a, 0x19FE); sta(a, 0x1800 + 0xC2)
    lda(a, 0x19FF); sta(a, 0x1800 + 0xC3)
    lda(a, 0x1900); sta(a, 0x1800 + 0xC4)
    lda(a, 0x1901); sta(a, 0x1800 + 0xC5)
    lda(a, 0x1902); sta(a, 0x1800 + 0xC6)
    lda(a, 0x1904); sta(a, 0x1800 + 0xC7)
    lda(a, 0x1905); sta(a, 0x1800 + 0xC8)
    lda(a, 0x1906); sta(a, 0x1800 + 0xC9)
    lda(a, 0x1908); sta(a, 0x1800 + 0xCA)
    lda(a, 0x18FF); sta(a, 0x1800 + 0xCB)
    a.bcta(3, "frame")
    return image(a)


def probe_pots(seed):
    """the pots, the keypads and the beam position read at every phase of the
    frame: each pass copies them to a different screen cell"""
    a = P.Asm()
    a.org(0)
    a.bcta(3, "start")
    a.org(0x20)
    a.label("start")
    a.cpsl(0x09)
    a.lodi(3, seed)
    setreg(a, 0x18FC, 0xFF)
    setreg(a, 0x19F9, 0x80 | 0x1B)
    a.lodi(1, 0)
    a.label("loop")
    lda(a, 0x19FF); stx(a, 0x1800)
    lda(a, 0x19FE); stx(a, 0x1810)
    lda(a, 0x1901); stx(a, 0x1820)
    lda(a, 0x1905); stx(a, 0x1830)
    lda(a, 0x18FF); stx(a, 0x1840)
    lda(a, 0x1908); stx(a, 0x1850)
    a.lodz(3); a.emit(0x84, 0x31); a.strz(3)       # R3 walks: flip the mux sometimes
    a.emit(0x44, 0x40); a.emit(0x64, 0x9B)
    sta(a, 0x19F9)
    a.lodz(1); a.emit(0x84, 1); a.emit(0x44, 0x0F); a.strz(1)
    a.bcta(3, "loop")
    return image(a)


PROBES = {
    "chaos1": lambda: probe_chaos(0x3B),
    "chaos2": lambda: probe_chaos(0x91),
    "chaos3": lambda: probe_chaos(0x07),
    "chaos4": lambda: probe_chaos(0x55),
    "chaos5": lambda: probe_chaos(0xA3),
    "chaos6": lambda: probe_chaos(0xE9),
    "chaos7": lambda: probe_chaos(0x12),
    "chaos8": lambda: probe_chaos(0x7D),
    "frames1": lambda: probe_frames(0x10),
    "frames2": lambda: probe_frames(0xC4),
    "frames3": lambda: probe_frames(0x6A),
    "frames4": lambda: probe_frames(0xF3),
    "pots": lambda: probe_pots(0x21),
}


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    for name, make in PROBES.items():
        with open(os.path.join(out, "uvi_%s.bin" % name), "wb") as f:
            f.write(make())
    print("wrote %d probes to %s" % (len(PROBES), out))


if __name__ == "__main__":
    main()
