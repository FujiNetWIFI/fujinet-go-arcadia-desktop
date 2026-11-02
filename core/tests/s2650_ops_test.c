/*
 * s2650_ops_test -- the 2650 core against MAME's, on the probe cartridges of
 * tools/ab/s2650_probes.py, in a model of the Arcadia's memory map.
 *
 *   s2650_ops_test                      the ctest: every probe in s2650_golden.h
 *                                       is run and its trace (length, FNV-1a
 *                                       hash of every row) compared with MAME's
 *   s2650_ops_test --run CART [MAXROWS [REF]]
 *                                       print the trace of one cartridge image,
 *                                       one row before every instruction, until
 *                                       the PC reaches $0FFD (tools/ab/
 *                                       s2650_ab.py diffs it with MAME's).
 *                                       REF is MAME's trace: the SENSE pin
 *                                       (the beam's vsync) is replayed from it
 *   s2650_ops_test --coverage CART      the opcodes the cartridge executes
 *
 * A row is the state before the instruction:   pc ps r0 r1 r2 r3 cycles
 * (ps = PSU << 8 | PSL; r1-r3 of the active register set).
 *
 * Memory map (MAME's arcadia driver with a 28K standard cartridge): ROM at
 * $0000-$0FFF = rom[0..FFF], $2000-$7FFF = rom[1000..6FFF]; the character RAM
 * $1800-$18EF; everything else open bus. Reads of $18F0-$1AFF (video
 * registers, controllers, the beam) or of RAM the probe has not written are
 * counted as hazards: a probe must have none.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s2650.h"

#define DONE_PC 0x0FFD

static const s2650_t *g_cpu;    /* for the SENSE model */

typedef struct {
    uint8_t rom[0x10000];
    size_t rom_size;
    uint8_t ram[0x300];
    uint8_t ram_valid[0x300];
    unsigned hazards;
    uint16_t hazard_addr;
    unsigned unmapped_reads;
    int unmapped_value;
    /* MAME's trace for the SENSE replay */
    const uint16_t *ref_ps;
    size_t ref_n;
    size_t row;
    uint32_t sense_calls;
    uint32_t sense_hits;
    uint64_t sense_model_miss;
    uint64_t sense_model_calls;
    uint64_t cycles_at_row;
    int use_model;
} machine_t;

static uint8_t mem_read(void *ctx, uint16_t a)
{
    machine_t *m = ctx;
    if (a < 0x1000)
        return a < m->rom_size ? m->rom[a] : 0xff;
    if (a >= 0x1800 && a < 0x1b00) {
        unsigned o = a - 0x1800;
        if (o >= 0xf0 || !m->ram_valid[o]) {
            m->hazards++;
            m->hazard_addr = a;
        }
        return m->ram[o];
    }
    if (a >= 0x2000) {
        size_t o = (size_t)a - 0x1000;
        return o < m->rom_size ? m->rom[o] : 0xff;
    }
    m->unmapped_reads++;
    return (uint8_t)m->unmapped_value;
}

static void mem_write(void *ctx, uint16_t a, uint8_t d)
{
    machine_t *m = ctx;
    if (a >= 0x1800 && a < 0x1b00) {
        m->ram[a - 0x1800] = d;
        m->ram_valid[a - 0x1800] = 1;
    }
}

static uint8_t io_read(void *ctx, int port)
{
    machine_t *m = ctx;
    (void)port;
    return (uint8_t)m->unmapped_value;
}

static void io_write(void *ctx, int port, uint8_t d)
{
    (void)ctx;
    (void)port;
    (void)d;
}

/* The SENSE pin is MAME's vsync_r: m_line >= 216, m_line counting 262 lines
 * per frame off a periodic timer of 262 * 60 Hz on the 895 kHz CPU clock.
 * MAME's timers advance between instructions, so an instruction sees the line
 * as of its start: line = floor((start_cycles + 0.85) * 15720 / 895000) mod 262.
 * (Fitted to 15000 SPSU + 15000 TPSU samples across 20 frames of MAME: zero
 * misses for an offset in [0.7, 1.0) clocks.) With a REF trace the pin is replayed from MAME's own PS
 * instead (the SI bit of the next row), to check the model. */
static int sense(void *ctx)
{
    machine_t *m = ctx;
    int model = (int)(((g_cpu->start_cycles * 100ull + 85ull) * 15720ull / (895000ull * 100ull)) % 262ull) >= 216;
    m->sense_calls++;
    if (m->ref_ps && m->row + 1 < m->ref_n) {
        int si = (m->ref_ps[m->row + 1] >> 15) & 1;
        m->sense_model_calls++;
        m->sense_model_miss += (unsigned)(si != model);
        return si;
    }
    return model;
}

static void setup(machine_t *m, s2650_t *c)
{
    s2650_init(c);
    c->ctx = m;
    c->read = mem_read;
    c->write = mem_write;
    c->io_read = io_read;
    c->io_write = io_write;
    c->sense = sense;
    s2650_reset(c);
}

typedef struct {
    size_t rows;
    int done;
    unsigned hazards;
    uint16_t hazard_addr;
    uint64_t hash;
    unsigned char seen[256];
} result_t;

#define FNV_OFFSET 0xCBF29CE484222325ULL
#define FNV_PRIME 0x100000001B3ULL

static uint64_t fnv(uint64_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * FNV_PRIME;
    return h;
}

/* run to DONE_PC or maxrows; print rows if `out` */
static void run(machine_t *m, size_t maxrows, FILE *out, result_t *res)
{
    static s2650_t cpu;
    char line[128];

    memset(res, 0, sizeof *res);
    res->hash = FNV_OFFSET;
    setup(m, &cpu);
    g_cpu = &cpu;
    while (res->rows < maxrows) {
        const uint16_t pc = s2650_pc(&cpu);
        const uint16_t ps = (uint16_t)((cpu.psu << 8) | cpu.psl);
        m->row = res->rows;
        snprintf(line, sizeof line, "%04X %04X %02X %02X %02X %02X %llu\n", pc, ps, cpu.reg[0], cpu.reg[1],
                 cpu.reg[2], cpu.reg[3], (unsigned long long)cpu.cycles);
        res->hash = fnv(res->hash, line);
        if (out)
            fputs(line, out);
        res->rows++;
        if (pc == DONE_PC) {
            res->done = 1;
            break;
        }
        s2650_step(&cpu);
        res->seen[cpu.ir] = 1;
    }
    res->hazards = m->hazards;
    res->hazard_addr = m->hazard_addr;
}

static int load_file(machine_t *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return -1;
    }
    m->rom_size = fread(m->rom, 1, sizeof m->rom, f);
    fclose(f);
    return 0;
}

static uint16_t *load_ref(const char *path, size_t *n)
{
    FILE *f = fopen(path, "r");
    char line[256];
    uint16_t *ps = NULL;
    size_t cap = 0;
    *n = 0;
    if (!f)
        return NULL;
    while (fgets(line, sizeof line, f)) {
        unsigned pc, p, r0, r1, r2, r3;
        unsigned long long cyc;
        if (sscanf(line, "%4x %4x %2x %2x %2x %2x %llu", &pc, &p, &r0, &r1, &r2, &r3, &cyc) == 7 &&
            strlen(line) > 20 && line[4] == ' ' && line[9] == ' ') {
            if (*n == cap) {
                cap = cap ? cap * 2 : 4096;
                ps = realloc(ps, cap * sizeof *ps);
            }
            ps[(*n)++] = (uint16_t)p;
        }
    }
    fclose(f);
    return ps;
}

#ifdef S2650_GOLDEN
#include "s2650_golden.h"
#endif

int main(int argc, char **argv)
{
    static machine_t m;
    result_t res;

    if (argc >= 3 && (strcmp(argv[1], "--run") == 0 || strcmp(argv[1], "--coverage") == 0)) {
        const int cov = strcmp(argv[1], "--coverage") == 0;
        size_t maxrows = argc > 3 ? (size_t)strtoull(argv[3], NULL, 0) : 10000000;
        if (getenv("S2650_UNMAPPED"))
            m.unmapped_value = (int)strtol(getenv("S2650_UNMAPPED"), NULL, 0);
        if (load_file(&m, argv[2]))
            return 2;
        if (argc > 4)
            m.ref_ps = load_ref(argv[4], &m.ref_n);
        run(&m, maxrows, cov ? NULL : stdout, &res);
        fprintf(stderr, "rows %zu done %d hazards %u (last $%04X) unmapped reads %u sense calls %u model misses %llu\n",
                res.rows, res.done, res.hazards, res.hazard_addr, m.unmapped_reads, m.sense_calls,
                (unsigned long long)m.sense_model_miss);
        if (cov) {
            int n = 0;
            for (int i = 0; i < 256; i++)
                if (res.seen[i]) {
                    printf("%02X ", i);
                    n++;
                }
            printf("\n%d opcodes\n", n);
        }
        return res.done ? 0 : 1;
    }
#ifdef S2650_GOLDEN
    {
        int failures = 0;
        for (unsigned i = 0; i < sizeof s2650_probes / sizeof s2650_probes[0]; i++) {
            memset(&m, 0, sizeof m);
            memset(m.rom, 0xc0, sizeof m.rom);
            m.rom_size = s2650_probes[i].size;
            for (unsigned j = 0; j < s2650_probes[i].nseg; j++)
                memcpy(m.rom + (s2650_probes[i].seg[j].addr < 0x1000 ? s2650_probes[i].seg[j].addr
                                                                     : s2650_probes[i].seg[j].addr - 0x1000),
                       s2650_probes[i].seg[j].data, s2650_probes[i].seg[j].len);
            run(&m, 100000000, NULL, &res);
            int ok = res.done && res.rows == s2650_probes[i].rows && res.hash == s2650_probes[i].hash &&
                     res.hazards == 0;
            printf("%-8s %7zu rows  hash %016llX  %s\n", s2650_probes[i].name, res.rows,
                   (unsigned long long)res.hash, ok ? "ok" : "FAIL");
            if (!ok) {
                printf("  expected %u rows, hash %016llX; done %d hazards %u\n", s2650_probes[i].rows,
                       (unsigned long long)s2650_probes[i].hash, res.done, res.hazards);
                failures++;
            }
        }
        return failures ? 1 : 0;
    }
#else
    fprintf(stderr, "usage: %s --run CART [MAXROWS [REF]] | --coverage CART\n", argv[0]);
    return 2;
#endif
}
