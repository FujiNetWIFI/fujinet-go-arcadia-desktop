#!/usr/bin/env python3
"""s2650_ab.py -- the 2650 core against MAME, on generated probe cartridges.

The probes (s2650_probes.py) are cartridge images for MAME's `arcadia` driver.
MAME runs each from reset with the debugger trace of s2650_trace.lua and logs
the CPU state before every instruction until the PC reaches $0FFD.
core/tests/s2650_ops_test runs the same images on core/arc/s2650.c in a model
of the Arcadia's memory map and prints the same rows.

    s2650_ab.py --gen [PROBE ...]     write core/tests/s2650_golden.h: the probe
                                      images plus, from MAME, each trace's length,
                                      FNV-1a hash and the SENSE pin's replay bits
                                      (committed, so the s2650_ops test needs
                                      neither MAME nor this script)
    s2650_ab.py --check [PROBE ...]   run MAME and the core live and diff the
                                      whole traces; --exe gives s2650_ops_test
    s2650_ab.py --mame PROBE          print MAME's trace for one probe
    s2650_ab.py --coverage [PROBE ...] which opcodes the probes execute (--exe)
    s2650_ab.py --list                the probes

Exit status: 0 equal, 1 a difference or a failure, 77 when MAME is missing
(ctest reads that as skipped).

Environment: MAME (default ~/Workspace/mame/mame). MAME is always run under
`timeout -s KILL`: it ignores SIGTERM with -video none.

SPDX-License-Identifier: GPL-3.0-or-later
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import s2650_probes as P          # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
GOLDEN = os.path.join(ROOT, "core", "tests", "s2650_golden.h")
LUA = os.path.join(HERE, "s2650_trace.lua")

LINE_RE = re.compile(r"^[0-9A-F]{4} [0-9A-F]{4}( [0-9A-F]{2}){4} \d+$")
FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
MASK64 = 0xFFFFFFFFFFFFFFFF


class Missing(Exception):
    pass


def find_mame():
    path = os.path.expanduser(os.environ.get("MAME", "~/Workspace/mame/mame"))
    if os.access(path, os.X_OK):
        return path
    found = shutil.which("mame")
    if found:
        return found
    raise Missing("mame not found (set $MAME)")


PROBES = {
    "alu": P.probe_alu,
    "arith": P.probe_arith,
    "addr": P.probe_addr,
    "wrap": P.probe_wrap,
}
for _n in ("branch", "psw", "illegal", "sense", "fuzz1", "fuzz2", "fuzz3", "fuzz4"):
    if hasattr(P, "probe_" + _n):
        PROBES[_n] = getattr(P, "probe_" + _n)


def probe_names(wanted):
    if not wanted:
        return list(PROBES)
    for w in wanted:
        if w not in PROBES:
            sys.exit("s2650_ab.py: no probe %s (have %s)" % (w, ", ".join(PROBES)))
    return wanted


def parse_trace(text):
    lines = []
    for raw in text.splitlines():
        raw = raw.strip()
        if not LINE_RE.match(raw):
            continue
        lines.append(raw)
        if int(raw[:4], 16) == P.DONE:
            return lines, True
    return lines, False


def mame_trace(name, image, work):
    mame = find_mame()
    run = os.path.join(work, name + "-mame")
    shutil.rmtree(run, ignore_errors=True)
    os.makedirs(run)
    cart = os.path.join(run, "cart.bin")
    with open(cart, "wb") as f:
        f.write(image)
    trace = os.path.join(run, "trace.txt")
    env = dict(os.environ, S2650_TRACE=trace, S2650_DONE="%X" % P.DONE, S2650_FRAMES="600")
    cmd = ["timeout", "-s", "KILL", "300", mame, "arcadia", "-cart", cart,
           "-video", "none", "-sound", "none", "-nothrottle", "-seconds_to_run", "120",
           "-debug", "-debugger", "none", "-autoboot_script", LUA, "-skip_gameinfo",
           "-cfg_directory", os.path.join(run, "cfg"),
           "-snapshot_directory", os.path.join(run, "snap"),
           "-nvram_directory", os.path.join(run, "nv")]
    r = subprocess.run(cmd, env=env, cwd=run, stdin=subprocess.DEVNULL,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if not os.path.exists(trace):
        sys.exit("s2650_ab.py: MAME wrote no trace for %s:\n%s" % (name, r.stdout[-2000:]))
    with open(trace) as f:
        lines, done = parse_trace(f.read())
    if not done:
        sys.exit("s2650_ab.py: %s never reached the end marker in MAME (%d lines)" % (name, len(lines)))
    return lines


def core_trace(exe, name, image, work, ref_path=None):
    path = os.path.join(work, name + ".bin")
    with open(path, "wb") as f:
        f.write(image)
    cmd = [exe, "--run", path, "100000000"] + ([ref_path] if ref_path else [])
    r = subprocess.run(cmd, capture_output=True, text=True)
    lines, done = parse_trace(r.stdout)
    if r.returncode != 0 or not done:
        sys.exit("s2650_ab.py: core run of %s failed: rc %d, %d lines: %s" %
                 (name, r.returncode, len(lines), r.stderr.strip()))
    return lines, r.stderr.strip()


def fnv(lines):
    h = FNV_OFFSET
    for line in lines:
        for b in (line + "\n").encode():
            h = ((h ^ b) * FNV_PRIME) & MASK64
    return h


def segments(image):
    """(cpu address, bytes) runs of the image that are not NOP fill, as the CPU
    sees them ($0000-$0FFF = rom[0..], $2000-$7FFF = rom[1000..])"""
    segs = []
    for base, lo, hi in ((0, 0, 0x1000), (0x2000, 0x1000, len(image))):
        i = lo
        while i < hi:
            if image[i] == P.FILL:
                i += 1
                continue
            j = i
            gap = 0
            while j < hi and gap < 24:
                gap = gap + 1 if image[j] == P.FILL else 0
                j += 1
            j -= gap
            segs.append((base + (i - lo), image[i:j]))
            i = j
    return segs


def compare(name, mame, core):
    n = min(len(mame), len(core))
    for i in range(n):
        if mame[i] != core[i]:
            lo = max(0, i - 3)
            print("%s: first difference at row %d" % (name, i))
            print("  pc   ps   r0 r1 r2 r3 cycles")
            for k in range(lo, min(n, i + 2)):
                print("%s MAME %s" % ("->" if k == i else "  ", mame[k]))
                print("%s core %s" % ("->" if k == i else "  ", core[k]))
            return False
    if len(mame) != len(core):
        print("%s: MAME has %d rows, core %d" % (name, len(mame), len(core)))
        return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("probes", nargs="*")
    ap.add_argument("--gen", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--mame", metavar="PROBE")
    ap.add_argument("--coverage", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--exe", default=os.path.join(ROOT, "build-cpu", "s2650_ops_test"))
    ap.add_argument("--keep", metavar="DIR", help="keep the work files here")
    a = ap.parse_args()

    if a.list:
        print("\n".join(PROBES))
        return 0
    work = a.keep or tempfile.mkdtemp(prefix="s2650ab-")
    os.makedirs(work, exist_ok=True)
    try:
        if a.mame:
            img = PROBES[a.mame]()
            print("\n".join(mame_trace(a.mame, img, work)))
            return 0
        names = probe_names(a.probes)
        if a.coverage:
            seen = set()
            for n in names:
                p = os.path.join(work, n + ".bin")
                open(p, "wb").write(PROBES[n]())
                r = subprocess.run([a.exe, "--coverage", p], capture_output=True, text=True)
                seen |= {int(x, 16) for x in r.stdout.splitlines()[0].split()}
            print("%d opcodes executed" % len(seen))
            missing = [i for i in range(256) if i not in seen]
            print("not executed: " + " ".join("%02X" % i for i in missing))
            return 0
        if a.check or a.gen:
            out = ["/* s2650_golden.h -- GENERATED by tools/ab/s2650_ab.py --gen: the probe",
                   " * cartridges of tools/ab/s2650_probes.py and, from MAME's s2650, each",
                   " * trace's row count and FNV-1a hash (of the rows, one per instruction,",
                   " * \"pc ps r0 r1 r2 r3 cycles\\n\"). s2650_ops_test holds core/arc/s2650.c",
                   " * to them. */", ""]
            bad = 0
            total = 0
            decls = []
            for n in names:
                img = PROBES[n]()
                mame = mame_trace(n, img, work)
                core, info = core_trace(a.exe, n, img, work)
                ok = compare(n, mame, core)
                print("%-8s %7d rows  %s   (%s)" % (n, len(mame), "equal" if ok else "DIFFERENT", info))
                total += len(mame)
                bad += 0 if ok else 1
                if a.gen and ok:
                    segs = segments(img)
                    out.append("static const uint8_t s2650_%s_img[] = {" % n)
                    flat = bytearray()
                    for _, d in segs:
                        flat += d
                    for i in range(0, len(flat), 16):
                        out.append("    " + ",".join("0x%02X" % b for b in flat[i:i + 16]) + ",")
                    out.append("};")
                    decls.append((n, segs, len(img), len(mame), fnv(mame)))
            print("%d rows in all, %d probe(s) differ" % (total, bad))
            if a.gen and not bad:
                out.append("static const struct {")
                out.append("    const char *name; unsigned size; unsigned rows; uint64_t hash;")
                out.append("    unsigned nseg; struct { uint16_t addr; uint16_t len; const uint8_t *data; } seg[48];")
                out.append("} s2650_probes[] = {")
                for n, segs, size, rows, h in decls:
                    off = 0
                    sl = []
                    for addr, d in segs:
                        sl.append("{0x%04X,%d,s2650_%s_img+%d}" % (addr, len(d), n, off))
                        off += len(d)
                    assert len(sl) <= 48, "%s has %d segments" % (n, len(sl))
                    out.append("    { \"%s\", %d, %d, 0x%016XULL, %d, { %s } }," %
                               (n, size, rows, h, len(sl), ", ".join(sl)))
                out.append("};")
                with open(GOLDEN, "w") as f:
                    f.write("\n".join(out) + "\n")
                print("wrote", GOLDEN)
            return 1 if bad else 0
        ap.print_help()
        return 2
    except Missing as e:
        print(e)
        return 77
    finally:
        if not a.keep:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
