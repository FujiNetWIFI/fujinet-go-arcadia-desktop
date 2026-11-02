/*
 * boot_smoke -- the paced host boots CONFIG and keeps real time.
 *
 * Starts the host thread (core/arc/host.h) on the baked CONFIG, waits for frames, and checks that CONFIG painted (MAME's
 * visible area, 192x262, with lit pixels on it) and that the frame rate is
 * the machine's own -- 60 Hz within 2%. The siblings' hardest-won lesson: verify the throttle (one once
 * free-ran at >33000%).
 *
 * The rate is only held to 2% where the OS can sleep for a frame without
 * gross overshoot: a loaded CI virtual machine (the macOS runners above
 * all) turns a 16.7 ms sleep into 50, which says nothing about the pacing
 * code. There the test checks what still means something -- the machine
 * is throttled, and the deadline ladder sleeps -- and says why, as the
 * Atari 2600 sibling's host test does.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "host.h"

static int failures;
static int accurate_sleeps = 1;
static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
}

static double now_s(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* MAME's line timer: attotime::from_hz(262 * 60) a line, 262 lines a frame */
static const double k_want_hz = 60.0;

static void run(void)
{
    static uint32_t fb[ARC_FB_WIDTH * ARC_FB_HEIGHT];
    const double want_hz = k_want_hz;
    arc_host_opts_t o;
    uint64_t serial = 0, f0, f1;
    int lit = 0;
    double t0, t1, hz;
    char what[160];

    memset(&o, 0, sizeof o);
    o.machine = ARC_MACHINE_ARCADIA;
    o.boip_hostport = "127.0.0.1:1";   /* no FujiNet: CONFIG runs link-down */
    if (arc_host_start(&o) != 0)
    {
        printf("FAIL: start: %s\n", arc_host_last_error());
        failures++;
        return;
    }

    for (int waited = 0; waited < 5000 && lit <= 200; waited += 100)
    {
        sleep_ms(100);
        if (!arc_host_frame_copy(fb, &serial))
            continue;
        lit = 0;
        for (int i = 0; i < ARC_FB_WIDTH * ARC_FB_HEIGHT; i++)
            if (fb[i] & 0xffffff)
                lit++;
    }
    check(serial != 0, "a frame was published");
    snprintf(what, sizeof what, "CONFIG painted (%d lit pixels)", lit);
    check(lit > 200, what);

    f0 = arc_host_frame_count();
    t0 = now_s();
    sleep_ms(2000);
    f1 = arc_host_frame_count();
    t1 = now_s();
    hz = (double)(f1 - f0) / (t1 - t0);
    snprintf(what, sizeof what, "the host reports %.4f Hz", arc_host_frame_rate());
    check(arc_host_frame_rate() > want_hz - 0.01 && arc_host_frame_rate() < want_hz + 0.01, what);

    arc_host_stop();
    {
        arc_host_pacing_t pc;
        arc_host_pacing(&pc);
        printf("  pacing: %llu sleeps, %.2f ms late on average, worst %.2f ms, %llu resyncs\n",
               (unsigned long long)pc.sleeps,
               pc.sleeps ? (double)pc.late_ns / (double)pc.sleeps / 1e6 : 0.0,
               (double)pc.worst_late_ns / 1e6, (unsigned long long)pc.resyncs);
        snprintf(what, sizeof what, "the machine is throttled: %.2f Hz, not free-running", hz);
        check(hz < want_hz * 1.10 && pc.sleeps > 0, what);
        snprintf(what, sizeof what, "it paces at %.2f Hz (want %.2f)", hz, want_hz);
        if (accurate_sleeps)
            check(hz > want_hz * 0.98 && hz < want_hz * 1.02, what);
        else
            printf("note: %s -- not asserted, this OS's sleeps are coarse\n", what);
    }
}

/* How fast this machine runs the core with no host and no throttle: the
 * pacing check means nothing on a runner that cannot keep up. */
static double capacity(void)
{
    static arc_machine_t m;
    char why[160];
    arc_machine_opts_t mo;
    arc_fujinet_t *fj = arc_fujinet_create();
    double t0, t1;
    const int frames = 120;

    /* the mailbox on its worker: inline, a link-down wait would stall the CPU */
    arc_fujinet_power_on(fj, NULL, 0, "127.0.0.1:1", false, why, sizeof why);
    memset(&mo, 0, sizeof mo);
    mo.machine = ARC_MACHINE_ARCADIA;
    mo.cart = arc_fujinet_cart(fj);
    mo.fujinet = fj;
    arc_machine_init(&m, &mo);
    t0 = now_s();
    for (int f = 0; f < frames; f++)
    {
        arc_machine_run_frame(&m);
        m.out_frames = 0;
    }
    t1 = now_s();
    arc_fujinet_destroy(fj);
    return frames / (t1 - t0);
}

int main(void)
{
    /* ten 16.7 ms sleeps: 167 ms on a machine that can sleep for a frame */
    {
        double t0 = now_s(), took;
        for (int i = 0; i < 10; i++)
            nanosleep(&(struct timespec){ 0, 16666667L }, NULL);
        took = now_s() - t0;
        accurate_sleeps = took < 0.25;
        printf("  ten 16.7 ms sleeps took %.3f s (%s)\n", took,
               accurate_sleeps ? "accurate" : "coarse: the rate is reported, not asserted");
    }
    /* before the host exists: the cart device is one per process */
    printf("  the bare core runs %.0f frames/s here\n", capacity());
    run();
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
