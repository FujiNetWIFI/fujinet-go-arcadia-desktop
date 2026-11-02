#!/usr/bin/env python3
"""s2650_probes.py -- generates the 2650 conformance probe cartridges.

No assembler is needed: a probe is built here from a tiny assembler (Asm) that
knows the 2650's operand formats. Each probe is an Arcadia cartridge image
(rom bytes; MAME's std slot maps rom[0..FFF] at $0000 and rom[1000..] at
$2000..$7FFF), run from reset until the PC reaches DONE. MAME's s2650 and our
core must trace identically (tools/ab/s2650_ab.py).

Memory the probes may touch: cartridge ROM, the UVI's character RAM
$1800-$18EF (read/write; initialised by the probe before it is read),
everything else reads as open bus. Probes never read $18F0-$18FF or $1900+
(video registers, inputs, beam position), never HALT, and never raise an
interrupt (the Arcadia driver has no IRQ source and Lua cannot set an input
line; interrupts are covered by core/tests/s2650_irq_test.c).

SPDX-License-Identifier: GPL-3.0-or-later
"""

import random

ROM_SIZE = 0x7000          # cart size: $2000-$7FFF all map to cartridge ROM
DONE = 0x0FFD              # BCTR,A self: the end marker
RAM = 0x1800               # character RAM base (R/W)
FILL = 0xC0                # NOP: a stray run slides along


def mapped(addr):
    """cartridge byte index for a CPU address, or None"""
    if addr < 0x1000:
        return addr
    if 0x2000 <= addr < 0x8000:
        return addr - 0x1000
    return None


def sx7(v):
    v &= 0x7F
    return v - 0x80 if v & 0x40 else v


class Asm:
    def __init__(self):
        self.mem = {}
        self.pc = 0
        self.labels = {}
        self.fix = []

    def org(self, a):
        self.pc = a

    def emit(self, *bs):
        for b in bs:
            assert mapped(self.pc) is not None, "emit at unmapped %04X" % self.pc
            assert self.pc not in self.mem, "overlap at %04X" % self.pc
            self.mem[self.pc] = b & 0xFF
            self.pc += 1

    def label(self, name):
        assert name not in self.labels, name
        self.labels[name] = self.pc

    def here(self):
        return self.pc

    # operand encoders
    def rel(self, op, target, ind=False):
        """op + 7-bit relative operand to target (label or address)"""
        self.emit(op)
        self.fix.append(("r", self.pc, self.pc - 1, target, ind))
        self.emit(0)

    def adr(self, op, target, ind=False):
        """op + 15-bit absolute branch address"""
        self.emit(op)
        self.fix.append(("a", self.pc, self.pc - 1, target, ind))
        self.emit(0, 0)

    def mem_a(self, op, addr, mode=0, ind=False):
        """absolute data reference (13-bit, in the instruction's page)"""
        self.emit(op)
        self.fix.append(("d", self.pc, self.pc - 1, addr, (mode, ind)))
        self.emit(0, 0)

    def resolve(self):
        def val(t):
            if isinstance(t, tuple):
                return (self.labels[t[0]] + t[1]) & 0x7FFF
            if isinstance(t, str):
                if t == "zb_ret" and t not in self.labels:
                    return DONE
                return self.labels[t]
            return t
        for kind, p, op_at, target, ind in self.fix:
            t = val(target)
            page = (op_at or 0) & 0x6000
            if kind == "imm":
                self.mem[p] = (t >> 8) & 0x7F if ind == "hi" else t & 0xFF
                continue
            if kind == "r":
                nxt = (p + 1) & 0x1FFF
                off = ((t & 0x1FFF) - nxt + 0x1000) & 0x1FFF
                off -= 0x1000
                assert -64 <= off <= 63, "relative out of range %04X -> %04X" % (op_at, t)
                assert (t & 0x6000) == page or ind, "relative leaves its page %04X -> %04X" % (op_at, t)
                self.mem[p] = (off & 0x7F) | (0x80 if ind else 0)
            elif kind == "a":
                assert 0 <= t < 0x8000
                self.mem[p] = ((t >> 8) & 0x7F) | (0x80 if ind else 0)
                self.mem[p + 1] = t & 0xFF
            else:
                mode, ind2 = ind
                assert (t & 0x6000) == page or ind2 or True
                self.mem[p] = ((t >> 8) & 0x1F) | (mode << 5) | (0x80 if ind2 else 0)
                self.mem[p + 1] = t & 0xFF

    def image(self):
        self.resolve()
        rom = bytearray([FILL]) * ROM_SIZE
        for a, b in self.mem.items():
            rom[mapped(a)] = b
        return bytes(rom)

    # ---- mnemonics ----
    def lodi(self, r, v): self.emit(0x04 + r, v)

    def lodi_imm(self, r, target, half):
        """lodi,r with the high or low byte of a label's address"""
        self.emit(0x04 + r)
        self.fix.append(("imm", self.pc, None, target, half))
        self.emit(0)
    def lodz(self, r): self.emit(0x00 + r)
    def strz(self, r): self.emit(0xC0 + r)
    def nop(self): self.emit(0xC0)
    def lpsl(self): self.emit(0x93)
    def lpsu(self): self.emit(0x92)
    def spsl(self): self.emit(0x13)
    def spsu(self): self.emit(0x12)
    def ppsl(self, v): self.emit(0x77, v)
    def cpsl(self, v): self.emit(0x75, v)
    def ppsu(self, v): self.emit(0x76, v)
    def cpsu(self, v): self.emit(0x74, v)
    def tpsl(self, v): self.emit(0xB5, v)
    def tpsu(self, v): self.emit(0xB4, v)
    def tmi(self, r, v): self.emit(0xF4 + r, v)
    def bctr(self, cc, t, ind=False): self.rel(0x18 + cc, t, ind)
    def bcfr(self, cc, t, ind=False): self.rel(0x98 + cc, t, ind)
    def bcta(self, cc, t, ind=False): self.adr(0x1C + cc, t, ind)
    def bcfa(self, cc, t, ind=False): self.adr(0x9C + cc, t, ind)
    def bstr(self, cc, t, ind=False): self.rel(0x38 + cc, t, ind)
    def bsfr(self, cc, t, ind=False): self.rel(0xB8 + cc, t, ind)
    def bsta(self, cc, t, ind=False): self.adr(0x3C + cc, t, ind)
    def bsfa(self, cc, t, ind=False): self.adr(0xBC + cc, t, ind)
    def retc(self, cc): self.emit(0x14 + cc)
    def rete(self, cc): self.emit(0x34 + cc)
    def brnr(self, r, t, ind=False): self.rel(0x58 + r, t, ind)
    def brna(self, r, t, ind=False): self.adr(0x5C + r, t, ind)
    def bsnr(self, r, t, ind=False): self.rel(0x78 + r, t, ind)
    def bsna(self, r, t, ind=False): self.adr(0x7C + r, t, ind)
    def birr(self, r, t, ind=False): self.rel(0xD8 + r, t, ind)
    def bira(self, r, t, ind=False): self.adr(0xDC + r, t, ind)
    def bdrr(self, r, t, ind=False): self.rel(0xF8 + r, t, ind)
    def bdra(self, r, t, ind=False): self.adr(0xFC + r, t, ind)
    def zbrr(self, off, ind=False): self.emit(0x9B, (off & 0x7F) | (0x80 if ind else 0))
    def zbsr(self, off, ind=False): self.emit(0xBB, (off & 0x7F) | (0x80 if ind else 0))
    def bxa(self, t, ind=False): self.adr(0x9F, t, ind)
    def bsxa(self, t, ind=False): self.adr(0xBF, t, ind)
    def dar(self, r): self.emit(0x94 + r)
    def rrl(self, r): self.emit(0xD0 + r)
    def rrr(self, r): self.emit(0x50 + r)


CLASSES = {  # name: base opcode
    "lod": 0x00, "eor": 0x20, "and": 0x40, "ior": 0x60, "add": 0x80, "sub": 0xA0,
    "str": 0xC0, "com": 0xE0,
}
PSL_COM, PSL_WC, PSL_C, PSL_RS, PSL_IDC, PSL_OVF = 0x02, 0x08, 0x01, 0x10, 0x20, 0x04


def start(a):
    """reset vector: jump over the page-zero stubs, init RAM, define stubs.
    Stubs at $0010 (RETC,A), $0020 (BCTA to the label 'zb_ret'), pointers at $0030."""
    a.org(0x0000)
    a.bcta(3, "start")
    a.org(0x0010)
    a.label("stub_ret")
    a.retc(3)
    a.org(0x0012)
    a.label("stub_ret2")
    a.nop()
    a.retc(3)
    a.org(0x0020)
    a.label("stub_jmp")
    a.bcta(3, RAM + 0xE0, ind=True)        # jump through the pointer the caller stored at $18E0
    a.org(0x0030)
    a.label("ptr_zb")           # ZBRR / ZBSR indirect targets
    a.emit(0x00, 0x20)          # -> stub_jmp
    a.emit(0x00, 0x10)          # -> stub_ret
    a.org(0x0040)
    a.label("start")
    # clear $1800-$18EF to a pattern
    a.lodi(1, 0xEF)
    a.label("init")
    a.lodz(1)
    a.emit(0x24, 0x5A)          # eori,r0 $5a  (r0 = r1 ^ 5a)
    a.mem_a(0xCD, RAM, 3)       # stra,r1 idx: ram[r1] = r0 ... (mode 3 uses R1 as index)
    a.bdrr(1, "init")
    a.lodz(1)                   # r1 = 0 here
    a.emit(0x24, 0x5A)
    a.mem_a(0xCD, RAM, 3)
    a.lodi(0, 0); a.lodi(1, 0); a.lodi(2, 0); a.lodi(3, 0)
    # keep the stack empty; no deeper setup


def done(a):
    a.org(DONE)
    a.label("done")
    a.bctr(3, "done")


def st(a, psl, regs):
    """PSL (RS=0 expected) and R0..R3 set; the last write is R0 so its CC rules"""
    a.lodi(1, regs[1]); a.lodi(2, regs[2]); a.lodi(3, regs[3])
    a.lodi(0, psl & 0xFF); a.lpsl()
    a.lodi(0, regs[0])


VALS = [0x00, 0x01, 0x02, 0x0F, 0x10, 0x3F, 0x40, 0x55, 0x7F, 0x80, 0x81, 0x99, 0xAA, 0xC0, 0xF0, 0xFF]


def pair_next(rng):
    return rng.choice(VALS), rng.choice(VALS)


class Seg:
    """sequential code segments over the cartridge's pages"""
    ORGS = [0x0100, 0x2000, 0x4000, 0x6000]
    LIMITS = [0x0FF0, 0x3F00, 0x5F00, 0x7F00]

    def __init__(self, a):
        self.a = a
        self.i = 0
        a.org(self.ORGS[0])
        self.n = 0

    def ensure(self, need):
        a = self.a
        if a.pc + need + 4 > self.LIMITS[self.i]:
            self.i += 1
            assert self.i < len(self.ORGS), "probe too big"
            a.bcta(3, "seg%d" % self.i)
            a.org(self.ORGS[self.i])
            a.label("seg%d" % self.i)


def data_byte(a, v):
    """bctr,a over one data byte; returns its address"""
    a.emit(0x1B, 0x01)
    d = a.pc
    a.emit(v)
    return d


def data_ptr(a, target):
    """bctr,a over a 2-byte big-endian pointer to target (label or addr); returns its address"""
    a.emit(0x1B, 0x02)
    d = a.pc
    a.fix.append(("p", d, None, target, None))
    a.emit(0, 0)
    return d


# patch the resolver for pointer data
_old_resolve = Asm.resolve


def _resolve(self):
    ptrs = [f for f in self.fix if f[0] == "p"]
    self.fix = [f for f in self.fix if f[0] != "p"]
    _old_resolve(self)
    for _, p, _, target, _ in ptrs:
        t = self.labels[target] if isinstance(target, str) else target
        self.mem[p] = (t >> 8) & 0x7F
        self.mem[p + 1] = t & 0xFF


Asm.resolve = _resolve


# ---------------------------------------------------------------------------
# probe: alu -- every class x form x register under assorted PSL states
# ---------------------------------------------------------------------------
PSLS = [0x00, PSL_WC, PSL_WC | PSL_COM | PSL_C, PSL_COM, PSL_C, PSL_WC | PSL_C, PSL_IDC | PSL_WC, PSL_OVF | PSL_COM]


def probe_alu(seed=1):
    rng = random.Random(seed)
    a = Asm()
    start(a)
    s = Seg(a)
    for cname, base in CLASSES.items():
        for form in "ZIRA":
            for r in range(4):
                if form == "Z" and cname in ("and", "str") and r == 0:
                    continue            # ANDZ,0 is HALT, STRZ,0 is NOP
                if form == "I" and cname == "str":
                    continue            # STRI is illegal
                for psl in rng.sample(PSLS, 4):
                    for _ in range(2):
                        s.ensure(70)
                        regs = [rng.choice(VALS) for _ in range(4)]
                        if form == "Z":
                            st(a, psl, regs)
                            a.emit(base + r)
                        elif form == "I":
                            st(a, psl, regs)
                            a.emit(base + 4 + r, rng.choice(VALS))
                        elif form == "R":
                            d = data_byte(a, rng.choice(VALS))
                            st(a, psl, regs)
                            a.rel(base + 8 + r, d)
                        else:
                            mode = rng.choice([0, 0, 1, 2, 3])
                            if cname == "str":
                                target = RAM + rng.randrange(0xE0)
                            else:
                                target = data_byte(a, rng.choice(VALS))
                            st(a, psl, regs)
                            if mode:
                                cur = regs[r]
                                nv = (cur + (1 if mode == 1 else -1 if mode == 2 else 0)) & 0xFF
                                addr = ((target - nv) & 0x1FFF) | (target & 0x6000)
                            else:
                                addr = target
                            a.mem_a(base + 0xC + r, addr, mode)
                            if cname == "str":
                                a.mem_a(0x0C, target, 0)      # read it back: loda,r0
    # DAR: every value x {C, IDC} states
    for pslv in (0x00, PSL_C, PSL_IDC, PSL_C | PSL_IDC, PSL_WC | PSL_C | PSL_IDC):
        s.ensure(40)
        a.lodi(0, pslv | PSL_COM); a.lpsl()
        a.lodi(1, 0)
        lbl = "dar%d" % a.pc
        a.label(lbl)
        a.lodz(1); a.strz(2)
        a.dar(2)
        a.dar(1)
        a.birr(1, lbl)
    # rotates: every value x WC/C
    for pslv in (0x00, PSL_C, PSL_WC, PSL_WC | PSL_C, PSL_WC | PSL_C | PSL_IDC):
        s.ensure(40)
        a.lodi(0, pslv); a.lpsl()
        a.lodi(1, 0)
        lbl = "rot%d" % a.pc
        a.label(lbl)
        a.lodz(1); a.strz(2); a.strz(3)
        a.rrl(2)
        a.rrr(3)
        a.rrl(2)
        a.rrr(3)
        a.birr(1, lbl)
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: arith -- ADDZ/SUBZ/COMZ/ANDZ/IORZ/EORZ over a value table x PSL states
# ---------------------------------------------------------------------------
def probe_arith():
    a = Asm()
    start(a)
    s = Seg(a)
    vt = [0x00, 0x01, 0x02, 0x07, 0x08, 0x0F, 0x10, 0x11, 0x3F, 0x40, 0x7F, 0x80, 0x81, 0xC0, 0xF0, 0xFF]
    s.ensure(40)
    a.bcta(3, "arith")
    a.label("vtab1")                # vtab1 + 1 .. + 16 : the table, indexed 16..1
    a.emit(0)
    for v in vt:
        a.emit(v)
    a.label("arith")
    ops = (0x80, 0xA0, 0xE0, 0x40, 0x60, 0x20)     # addz subz comz andz iorz eorz (+3: r3)
    pslt = [0x00, PSL_C, PSL_WC, PSL_WC | PSL_C, PSL_COM, PSL_COM | PSL_C, PSL_COM | PSL_WC, PSL_COM | PSL_WC | PSL_C]
    n = 0
    for op in ops:
        for p in pslt:
            n += 1
            s.ensure(40)
            a.lodi(0, p); a.lpsl()
            a.lodi(1, 16)
            lo = "ao%d" % n
            li = "ai%d" % n
            a.label(lo)
            a.mem_a(0x0D, "vtab1", 3)        # loda,r1 idx(R1): r0 = vtab1[r1]
            a.strz(3)                        # r3 = a
            a.lodi(2, 16)
            a.label(li)
            a.mem_a(0x0E, "vtab1", 3)        # loda,r2 idx(R2): r0 = b
            a.emit(op + 3)                   # op,r3: r0 = b op a
            a.bdrr(2, li)
            a.bdrr(1, lo)
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: addr -- addressing modes, indirection, indexing
# ---------------------------------------------------------------------------
def probe_addr(seed=3):
    rng = random.Random(seed)
    a = Asm()
    start(a)
    s = Seg(a)
    s.ORGS = [0x0100, 0x2002, 0x4002, 0x6002]
    s.LIMITS = [0x0F00, 0x3E00, 0x5E00, 0x7E00]
    for cname in ("lod", "add", "com", "ior", "str", "eor"):
        base = CLASSES[cname]
        for form in "RA":
            for r in range(4):
                for rep in range(8):
                    s.ensure(80)
                    regs = [rng.choice(VALS) for _ in range(4)]
                    ind = rng.random() < 0.4
                    mode = rng.choice([0, 1, 2, 3]) if form == "A" else 0
                    if cname == "str" and form == "A":
                        tgt = RAM + rng.randrange(0xE0)
                    else:
                        tgt = data_byte(a, rng.choice(VALS))
                    if form == "R":
                        if ind:
                            p = data_ptr(a, tgt)
                            st(a, PSL_COM, regs)
                            a.rel(base + 8 + r, p, ind=True)
                        else:
                            st(a, PSL_COM, regs)
                            a.rel(base + 8 + r, tgt)
                    else:
                        cur = regs[r]
                        nv = (cur + (1 if mode == 1 else -1 if mode == 2 else 0)) & 0xFF
                        adj = nv if mode else 0
                        ba = ((tgt - adj) & 0x1FFF) | (tgt & 0x6000)
                        if ind:
                            p = data_ptr(a, ba)
                            st(a, PSL_COM, regs)
                            a.mem_a(base + 0xC + r, p, mode, ind=True)
                        else:
                            st(a, PSL_COM, regs)
                            a.mem_a(base + 0xC + r, ba, mode)
                        if cname == "str":
                            a.mem_a(0x0C, tgt, 0)       # read it back: loda,r0
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: wrap -- operands, pointers and indexes that cross an 8K page edge
# ---------------------------------------------------------------------------
def probe_wrap():
    """Page 1 ($2000), 2 ($4000), 3 ($6000) each end with an instruction whose
    operand bytes wrap to the page start, then continue there."""
    a = Asm()
    start(a)
    a.org(0x0100)
    a.lodi(1, 0x10); a.lodi(2, 0x20); a.lodi(3, 0x30)
    a.bcta(3, "p1tail")
    # ---- page 1: 3-byte loda,r1 at B+1FFE: hr at B+1FFF, dr wraps to B+0
    B = 0x2000
    a.org(B + 0x40); a.label("d1"); a.emit(0x6B)
    a.org(B + 0x1FFE); a.label("p1tail"); a.emit(0x0D, ((B + 0x40) >> 8) & 0x1F)
    a.org(B + 0); a.emit((B + 0x40) & 0xFF)
    # execution continues at B+1
    a.org(B + 1)
    a.lodi(2, 0x44)
    a.bcta(3, "p2tail")
    # ---- page 2: adda,r1 at B+1FFF: hr, dr wrap to B+0, B+1
    B = 0x4000
    a.org(B + 0x41); a.label("d2"); a.emit(0x94)
    a.org(B + 0x1FFF); a.label("p2tail"); a.emit(0x8D)
    a.org(B + 0); a.emit(((B + 0x41) >> 8) & 0x1F, (B + 0x41) & 0xFF)
    a.org(B + 2)
    a.lodi(3, 0x55)
    a.bcta(3, "p3tail")
    # ---- page 3: lodr,r2 at B+1FFE (operand at B+1FFF), then bctr,a with its operand wrapped
    B = 0x6000
    a.org(B + 0x1FFE); a.label("p3tail"); a.emit(0x0A)
    a.fix.append(("r", a.pc, a.pc - 1, "d3", False)); a.emit(0)         # operand at B+1FFF -> d3 at B+0x20
    # iar wrapped to 0: next opcode at B+0
    a.org(B + 0x20); a.label("d3"); a.emit(0xA7)
    a.org(B + 0)
    a.emit(0x04, 0x66)                                                    # lodi,r0 $66
    a.bcta(3, "b3")
    a.org(B + 0x50); a.label("b3")
    a.lodi(1, 0x77)
    a.lodi(2, 0xE0)
    # indexed abs near the page end: ea = B+1FF0 + R2 wraps inside the page
    a.org(B + 0xD0); a.label("d5"); a.emit(0x2D)
    a.org(B + 0xD1); a.emit(0x3E)
    a.org(B + 0xCF); a.emit(0x4F)
    a.org(B + 0x70)
    a.mem_a(0x0E, B + 0x1FF0, 3)           # loda,r2 idx R2: ea = B+0x1FF0+0xE0 -> wraps within the page to B+0xD0
    a.mem_a(0x0E, B + 0x1FF0, 1)           # auto-inc: R2 -> 0xE1 : B+0xD1
    a.mem_a(0x0E, B + 0x1FF0, 2)           # auto-dec twice
    a.mem_a(0x0E, B + 0x1FF0, 2)
    # a pointer straddling the page end: high byte at B+1FFF (the lodr operand above),
    # low byte wraps to B+0 (the lodi opcode, $04)
    a.mem_a(0x0C, B + 0x1FFF, 0, ind=True)          # loda,r0 *ptr
    a.mem_a(0x0D, B + 0x1FFF, 2, ind=True)          # loda,r1 *ptr,r1-
    a.bcta(3, "tail3")
    a.org(B + 0x1FC0); a.label("tail3")
    a.lodi(1, 0x3D)
    a.rel(0x09, B + 0x1FFF, ind=True)               # lodr,r1 *ptr  (relative, indirect, wrapping)
    a.rel(0x0A, B + 0x1FFF, ind=True)
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: branch -- every branch, call and return form, taken and not taken
# ---------------------------------------------------------------------------
class Uid:
    def __init__(self):
        self.n = 0

    def __call__(self, pfx="L"):
        self.n += 1
        return "%s%d" % (pfx, self.n)


CCV = (0x00, 0x01, 0x80)         # R0 values giving CC zero / plus / minus


def probe_branch(seed=5):
    rng = random.Random(seed)
    a = Asm()
    start(a)
    s = Seg(a)
    U = Uid()

    def setcc(v):
        a.lodi(0, v)

    for rep in range(2):
        for cc in range(4):
            for v in CCV:
                # conditional forward branches: relative and absolute, true and false
                for form in ("bctr", "bcta", "bcfr", "bcfa"):
                    if form.startswith("bcf") and cc == 3:
                        continue            # ZBRR / BXA
                    s.ensure(30)
                    t = U()
                    setcc(v)
                    getattr(a, form)(cc, t)
                    a.lodi(1, 0xA1)
                    a.label(t)
                    a.lodi(2, 0xA2)
                # calls
                for form in ("bstr", "bsta", "bsfr", "bsfa"):
                    if form.startswith("bsf") and cc == 3:
                        continue            # ZBSR / BSXA
                    s.ensure(40)
                    sub, end = U("sub"), U("end")
                    setcc(v)
                    getattr(a, form)(cc, sub)
                    a.bctr(3, end)
                    a.label(sub)
                    a.lodi(3, 0xB3)
                    a.retc(3)
                    a.label(end)
                # conditional returns
                for op in ("retc", "rete"):
                    s.ensure(50)
                    sub, end = U("sub"), U("end")
                    a.bsta(3, sub)
                    a.bctr(3, end)
                    a.label(sub)
                    setcc(v)
                    getattr(a, op)(cc)
                    a.lodi(1, 0xC1)
                    a.retc(3)
                    a.label(end)
        # register-conditional branches, loops
        for r in range(4):
            for v in (0, 1, 0xFF, 0x80):
                s.ensure(120)
                t1, t2, sub, end = U(), U(), U("sub"), U("end")
                a.lodi(r, v)
                a.brnr(r, t1)
                a.lodi(1 if r != 1 else 2, 0xD1)
                a.label(t1)
                a.lodi(r, v)
                a.brna(r, t2)
                a.lodi(1 if r != 1 else 2, 0xD2)
                a.label(t2)
                a.lodi(r, v)
                a.bsnr(r, sub)
                a.bctr(3, end)
                a.label(sub)
                a.nop()
                a.retc(3)
                a.label(end)
                sub2, end2 = U("sub"), U("end")
                a.lodi(r, v)
                a.bsna(r, sub2)
                a.bctr(3, end2)
                a.label(sub2)
                a.nop()
                a.retc(3)
                a.label(end2)
            for form in ("bdrr", "bdra", "birr", "bira"):
                s.ensure(30)
                lp = U("lp")
                a.lodi(r, 4 if form.startswith("bd") else 0xFC)
                a.label(lp)
                a.nop()
                getattr(a, form)(r, lp)
            # brnr / brna loops
            for form in ("brnr", "brna"):
                s.ensure(30)
                lp = U("lp")
                a.lodi(r, 3)
                a.label(lp)
                a.emit(0xA4 + r, 1)           # subi,r 1
                getattr(a, form)(r, lp)
    # indirect forms
    for cc in range(4):
        for form in ("bctr", "bcta", "bstr", "bsta"):
            s.ensure(60)
            t, end, sub = U(), U("end"), U("sub")
            if form in ("bctr", "bcta"):
                p = data_ptr(a, t)
                setcc(CCV[cc % 3])
                getattr(a, form)(cc, p, ind=True)
                a.lodi(1, 0xE1)
                a.label(t)
                a.lodi(2, 0xE2)
            else:
                p = data_ptr(a, sub)
                setcc(CCV[cc % 3])
                getattr(a, form)(cc, p, ind=True)
                a.bctr(3, end)
                a.label(sub)
                a.retc(3)
                a.label(end)
    # ZBRR / ZBSR (page-zero relative, direct and through the pointers at $30/$32)
    for kind in ("zbrr", "zbrr_i", "zbsr", "zbsr_i"):
        for rep in range(2):
            s.ensure(60)
            t = U()
            if kind.startswith("zbrr"):
                # the stub at $20 jumps through the pointer at $18E0/$18E1
                a.lodi_imm(0, t, "hi")
                a.mem_a(0xCC, RAM + 0xE0, 0)             # stra,r0 $18E0
                a.lodi_imm(0, t, "lo")
                a.mem_a(0xCC, RAM + 0xE1, 0)
                if kind == "zbrr":
                    a.zbrr(0x20)
                else:
                    a.zbrr(0x30, ind=True)
                a.lodi(1, 0xF1)
                a.label(t)
                a.lodi(2, 0xF2)
            else:
                if kind == "zbsr":
                    a.zbsr(0x10)
                else:
                    a.zbsr(0x32, ind=True)
                a.lodi(1, 0xF3)
    # BXA / BSXA (indexed by R3)
    for k in (0, 3, 0x20, 0xFF):
        for kind in ("bxa", "bsxa"):
            s.ensure(60)
            t, end = U(), U("end")
            a.lodi(3, k)
            if kind == "bxa":
                a.adr(0x9F, (t, -k))
                a.lodi(1, 0xF4)
                a.label(t)
                a.lodi(2, 0xF5)
            else:
                a.adr(0xBF, (t, -k))
                a.bctr(3, end)
                a.label(t)
                a.lodi(2, 0xF6)
                a.retc(3)
                a.label(end)
            # (bxa with R3 = 0xFF reaches t - 0xFF + 0xFF)
    # the 8-deep return stack (SP wraps through 0)
    s.ensure(60)
    rec, more, end = U("rec"), U("more"), U("end")
    a.lodi(1, 8)
    a.bsta(3, rec)
    a.bctr(3, end)
    a.label(rec)
    a.bdrr(1, more)
    a.retc(3)
    a.label(more)
    a.bsta(3, rec)
    a.retc(3)
    a.label(end)
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: psw -- PSU/PSL instructions, register banks, TMI, I/O
# ---------------------------------------------------------------------------
def probe_psw(seed=7):
    rng = random.Random(seed)
    a = Asm()
    start(a)
    s = Seg(a)
    masks = [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xFF, 0x03, 0x0C, 0x18, 0x38, 0x7F, 0xC0,
             0x55, 0xAA, 0x07, 0x1F]
    psu_ops = (0x74, 0x76, 0xB4)          # cpsu ppsu tpsu
    psl_ops = (0x75, 0x77, 0xB5)          # cpsl ppsl tpsl
    # the register banks: distinct values in both, switched by ppsl/cpsl/lpsl
    for rep in range(3):
        s.ensure(200)
        a.lodi(0, 0); a.lpsl()                 # RS = 0
        a.lodi(1, 0x11); a.lodi(2, 0x12); a.lodi(3, 0x13)
        a.ppsl(PSL_RS)                         # bank 1
        a.lodi(1, 0x21); a.lodi(2, 0x22); a.lodi(3, 0x23)
        a.lodz(1); a.emit(0x81)                # addz,r1
        a.cpsl(PSL_RS)                         # back to bank 0
        a.lodz(1); a.emit(0x82)
        a.lodi(0, PSL_RS | PSL_WC); a.lpsl()   # via LPSL
        a.lodz(2)
        a.lodi(0, 0); a.lpsl()
        a.lodz(2)
        a.ppsl(PSL_RS); a.ppsl(PSL_RS)         # set twice: swap once
        a.lodz(3)
        a.cpsl(PSL_RS); a.cpsl(PSL_RS)
        a.lodz(3)
        a.cpsl(0xFF)                           # clears everything, RS included
        a.ppsl(0xFF)
        a.cpsl(0xFF)
    # random PSU / PSL / test sequences
    for n in range(260):
        s.ensure(40)
        k = rng.random()
        if k < 0.25:
            a.emit(rng.choice(psu_ops), rng.choice(masks))
        elif k < 0.5:
            a.emit(rng.choice(psl_ops), rng.choice(masks))
        elif k < 0.58:
            a.lodi(0, rng.choice(masks)); a.lpsl()
        elif k < 0.66:
            a.lodi(0, rng.choice(masks)); a.lpsu()
        elif k < 0.74:
            a.spsl()
        elif k < 0.82:
            a.spsu()
        elif k < 0.9:
            a.tmi(rng.randrange(4), rng.choice(masks))
        else:
            a.lodi(rng.randrange(4), rng.choice(masks))
    # make sure SP is back to 0 for the clean finish
    a.cpsu(0x07)
    # TMI sweep: every value x a few masks
    for m in (0x01, 0x80, 0x55, 0xFF, 0x00):
        s.ensure(30)
        lp = "tmi%d" % a.pc
        a.lodi(1, 0)
        a.label(lp)
        a.tmi(1, m)
        a.birr(1, lp)
    # I/O instructions: control, data, extended
    for r in range(4):
        for port in (0x00, 0x01, 0x7F, 0xFF):
            s.ensure(30)
            a.lodi(r, 0x5A ^ port)
            a.emit(0xD4 + r, port)             # wrte,r port
            a.emit(0x54 + r, port)             # rede,r port
        a.lodi(r, 0xA5)
        a.emit(0xB0 + r)                       # wrtc,r
        a.emit(0x30 + r)                       # redc,r
        a.emit(0xF0 + r)                       # wrtd,r
        a.emit(0x70 + r)                       # redd,r
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: sense -- SPSU/TPSU polled across ~20 frames (the SENSE pin = vsync)
# ---------------------------------------------------------------------------
def probe_sense():
    a = Asm()
    start(a)
    s = Seg(a)
    a.lodi(2, 100)
    a.label("so")
    a.lodi(1, 150)
    a.label("si")
    a.spsu()
    a.tpsu(0x80)
    a.bdrr(1, "si")
    a.bdrr(2, "so")
    a.bcta(3, "done")
    done(a)
    return a.image()


# ---------------------------------------------------------------------------
# probe: fuzz -- random straight-line blocks with forward branches, looped
# ---------------------------------------------------------------------------
def fuzz_op(a, rng, s):
    """one random non-control instruction. Stores only go to the character RAM through
    a ROM pointer (never the loop counter at $18EE); loads may use any addressing mode."""
    k = rng.random()
    base = rng.choice([0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xE0, 0xC0, 0x80, 0xA0, 0xE0])
    r = rng.randrange(4)
    if k < 0.20:                                        # register form
        if base == 0x40 and r == 0:
            r = 1
        a.emit(base + r)
    elif k < 0.42:                                      # immediate
        if base == 0xC0:
            base = 0x80
        a.emit(base + 4 + r, rng.randrange(256))
    elif k < 0.58:                                      # relative (never indirect for stores)
        off = rng.randrange(256)
        if base == 0xC0:
            off &= 0x7F
        a.emit(base + 8 + r, off)
    elif k < 0.76:                                      # absolute, any mode for loads
        hr, dr = rng.randrange(256), rng.randrange(256)
        if base == 0xC0:
            # store through a RAM pointer, direct (no index)
            tgt = RAM + rng.randrange(0xD0)
            p = data_ptr(a, tgt)
            a.mem_a(0xCC + r, p, 0, ind=True)
        else:
            a.emit(base + 0xC + r, hr, dr)
    elif k < 0.80:
        a.emit(rng.choice((0x50, 0xD0)) + r)           # rrr / rrl
    elif k < 0.84:
        a.emit(0x94 + r)                                # dar
    elif k < 0.88:
        a.emit(rng.choice((0xF4, 0xE4, 0x44)) + r, rng.randrange(256))   # tmi / comi / andi
    elif k < 0.91:
        a.emit(rng.choice((0x12, 0x13)))               # spsu spsl
    elif k < 0.94:
        # psl/psu tests and presets (RS and WC/COM toggles included)
        op = rng.choice((0x75, 0x77, 0xB5, 0x74, 0x76, 0xB4))
        m = rng.choice((0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x18, 0x0A, 0xFF, 0x22, 0x28))
        if op in (0x74, 0x76) and rng.random() < 0.5:
            m &= ~0x07                                  # leave SP mostly alone
        a.emit(op, m)
    elif k < 0.96:
        a.emit(rng.choice((0x30, 0x70, 0xB0, 0xF0)) + r)    # redc redd wrtc wrtd
    elif k < 0.98:
        a.emit(rng.choice((0x54, 0xD4)) + r, rng.randrange(256))   # rede wrte
    else:
        a.emit(rng.choice((0x93, 0x92)))                # lpsl lpsu


def probe_fuzz(seed, nblocks=60, iters=14, bias=0.0):
    rng = random.Random(seed)
    a = Asm()
    start(a)
    # main: counter in RAM $18EE
    a.org(0x0100)
    a.lodi(0, iters)
    a.mem_a(0xCC, RAM + 0xEE, 0)                        # stra,r0 $18EE
    a.bcta(3, "top")
    s = Seg(a)
    s.ORGS = [0x2000, 0x4000, 0x6000]
    s.LIMITS = [0x3E00, 0x5E00, 0x7E00]
    a.org(0x2000)
    a.label("top")
    a.lodi(0, 0x00); a.lodi(1, rng.randrange(256)); a.lodi(2, rng.randrange(256)); a.lodi(3, rng.randrange(256))
    labels = []
    for b in range(nblocks):
        s.ensure(100)
        lbl = "blk%d" % b
        a.label(lbl)
        for _ in range(rng.randrange(5, 13)):
            fuzz_op(a, rng, s)
        if rng.random() < 0.5 and b + 2 < nblocks:
            # forward conditional branch over the next block(s)
            tgt = "blk%d" % (b + 1 + rng.randrange(1, 3) - 1)
            tgt = "blk%d" % min(nblocks - 1, b + rng.randrange(1, 4))
            form = rng.choice(("bcta", "bcfa", "brna", "bdra", "bira"))
            if form in ("bcta", "bcfa"):
                getattr(a, form)(rng.randrange(3), tgt)
            else:
                getattr(a, form)(rng.randrange(4), tgt)
    s.ensure(100)
    a.label("end_blocks")
    # loop control through RAM (reached by a pointer: absolute data refs are page-local)
    pc_ = data_ptr(a, RAM + 0xEE)
    a.mem_a(0x0C, pc_, 0, ind=True)                     # loda,r0 *ptr
    a.cpsl(PSL_WC)                                      # (no borrow)
    a.emit(0xA4, 1)                                     # subi,r0 1
    a.mem_a(0xCC, pc_, 0, ind=True)
    a.bcfa(0, "top")
    a.bcta(3, "done")
    done(a)
    return a.image()


# the fuzz probes: seeds chosen so that none reads a hazardous address
def probe_fuzz1(): return probe_fuzz(2, nblocks=220, iters=40)
def probe_fuzz2(): return probe_fuzz(4, nblocks=220, iters=40)
def probe_fuzz3(): return probe_fuzz(10, nblocks=220, iters=40)
def probe_fuzz4(): return probe_fuzz(16, nblocks=220, iters=40)


# ---------------------------------------------------------------------------
# probe: illegal -- the ten undefined opcodes (6 clocks, no effect)
# ---------------------------------------------------------------------------
def probe_illegal():
    a = Asm()
    start(a)
    a.org(0x0100)
    for op in (0x10, 0x11, 0x90, 0x91, 0xB6, 0xB7, 0xC4, 0xC5, 0xC6, 0xC7):
        a.lodi(0, op); a.lodi(1, op ^ 0xFF)
        a.emit(op)
        a.emit(0xC0)
    a.bcta(3, "done")
    done(a)
    return a.image()
