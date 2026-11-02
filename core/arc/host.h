/*
 * host.h -- the emulator host: owns the machine and the FujiNet cartridge,
 * runs the machine on its own thread at real speed, and publishes video
 * frames and audio for the session layer.
 *
 * Process-singleton by design, like every core in this family (and the
 * FujiNet cart below it is one too: fujimail's port interface is context-
 * free C function pointers). arc_host_start refuses a second concurrent
 * session.
 *
 * Pacing: the machine runs a whole frame (to the next VBLANK) at a time
 * against an absolute monotonic deadline ladder at the machine's exact frame
 * rate -- NTSC 60 Hz, PAL 50 Hz. While a frontend feeds display ticks
 * at that rate (arc_host_notify_vsync) the machine phase-locks one frame per
 * tick instead (only possible near 60 Hz: a 50 Hz machine on a 60 Hz display
 * stays on the wall clock). The siblings' hardest-won lesson applies: VERIFY
 * the pacing -- the boot_smoke test measures it.
 *
 * Energy: a frame identical to its predecessor is not republished, the frame
 * callback tells a frontend when there is something to draw, and the machine
 * sleeps to its deadline rather than spinning.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARC_HOST_H
#define ARC_HOST_H

#include <stdbool.h>
#include <stdint.h>

#include "arc_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int machine;                 /* ARC_MACHINE_* (arc_internal.h) */

    const uint8_t *cart;         /* an opened image, copied; NULL boots CONFIG */
    uint32_t cart_size;
    int cart_type;               /* ARC_CART_* (carts.h) from arc_cart_identify */
    bool cart_app;               /* a FujiNet app (claims the mailbox): served
                                    by the FujiNet cartridge, mailbox live.
                                    Otherwise a game: the mapper cart for
                                    cart_type sits in the slot instead, as a
                                    real cartridge would, and FujiNet is not
                                    reachable until the next power cycle. */
    const char *boip_hostport;   /* fujinet-pc's BoIP listener, "host:port" */
    bool cart_sync;              /* run the mailbox inline (tests) */
} arc_host_opts_t;

/* Start the machine thread. Returns 0, or -1 with arc_host_last_error()
 * set (already running, an image the cartridge cannot map, allocation or
 * thread failure). */
int arc_host_start(const arc_host_opts_t *opts);

/* Stop and join the machine thread and power the cart off. Safe when not
 * running. */
void arc_host_stop(void);

bool arc_host_is_running(void);
const char *arc_host_last_error(void);

/* Pause: the machine thread parks between frames and the audio goes silent
 * (the session's pause; the debugger's own stop is separate). */
void arc_host_set_paused(bool paused);
bool arc_host_is_paused(void);

/* ---- video: latest-wins single slot ----
 * Copies the newest frame into dst (ARC_FB_WIDTH x ARC_FB_HEIGHT XRGB words)
 * when its serial differs from *serial_inout; returns nonzero and updates the
 * serial when it copied. */
int arc_host_frame_copy(uint32_t *dst, uint64_t *serial_inout);
/* Called on the machine thread right after each new frame is published, so a
 * frontend with no frame clock can draw when there is something to draw
 * instead of polling. Must be quick and must not call back into the host (it
 * runs with a lock held: set_frame_callback(NULL, NULL) waits for a call in
 * flight, so the context may be freed once it returns). */
typedef void (*arc_host_frame_cb)(void *ctx);
void arc_host_set_frame_callback(arc_host_frame_cb cb, void *ctx);
/* Frames the machine has run since start (for pacing checks). */
uint64_t arc_host_frame_count(void);
/* How the deadline ladder fared (boot_smoke prints it): sleeps taken, how
 * late they woke in total and at worst, and resyncs after falling more
 * than four frames behind. Read after arc_host_stop. */
typedef struct {
    uint64_t sleeps, late_ns, resyncs;
    long worst_late_ns;
} arc_host_pacing_t;
void arc_host_pacing(arc_host_pacing_t *out);
/* The running machine's exact frame rate in Hz (0 when stopped). */
double arc_host_frame_rate(void);

void arc_host_notify_vsync(int64_t frame_time_ns);

/* ---- audio: 48 kHz interleaved stereo float (mono mirrored), self-
 * correcting backlog. Returns frames written (no padding). */
int arc_host_audio_copy(float *dst, int max_frames);
void arc_host_audio_reset(void);

/* ---- inputs (callable from any thread; latched by the machine once a
 * frame, at VBLANK, as MAME's input ports are) ---- */
/* The key matrix: bit i set = held. Bits 0-11 player 1's keypad, 12-23
 * player 2's, each in this order: 1 2 3 4 5 6 7 8 9 CLEAR 0 ENTER; bit 24
 * START, 25 OPTION, 26 SELECT (the console panel, $1908 b0-b2). */
void arc_host_keys_set(uint64_t matrix);
/* A stick, digital: bit 0 UP, 1 RIGHT, 2 DOWN, 3 LEFT, 4 FIRE. FIRE is the
 * keypad's "2" wire ($1901/$1905 bit 3): the machine ORs it in. The pot goes to
 * its end for a direction, as MAME's joystick port does. */
void arc_host_stick_set(int stick, uint8_t bits);
/* A stick, analog (the disc): x and y -128..127, 0 = centre. When either is
 * non-zero the pot follows the disc and the digital bits are ignored. */
void arc_host_disc_set(int stick, int x, int y);
/* The console's RESET key: the 2650's /RESET, held while down. */
void arc_host_reset_set(bool down);
/* Pressed for a few frames and released (the menus' and the controllers
 * window's RESET when it is clicked rather than held). */
void arc_host_reset_pulse(void);
void arc_host_release_all(void);

/* ---- the cartridge ---- */
void arc_host_cart_status(arc_cart_status_t *out);
/* The cart device, or NULL when not running (the debugger's page map and
 * mailbox views). Use it under arc_host_lock. */
arc_cart_t *arc_host_cart(void);

/* ---- the run lock ----
 * Held by the machine thread while it runs a frame. Any other thread that
 * reads or edits the machine or the cart (arc_host_machine, arc_host_cart)
 * holds it around the access: it waits for the frame in progress, at most a
 * couple of milliseconds. Re-entrant per thread; a no-op on the machine
 * thread. Never call arc_host_stop while holding it. */
void arc_host_lock(void);
void arc_host_unlock(void);
/* The debugger's park: the machine thread lets go of the run lock while it
 * is parked inside the instruction hook, and takes it back to run on. */
void arc_host_park_release(void);
void arc_host_park_reacquire(void);

/* ---- debugger plumbing ----
 * The live machine; valid only while running, and only touched under
 * arc_host_lock (or on the machine thread). */
arc_machine_t *arc_host_machine(void);

/* Instruction-boundary hook (machine thread; may block). */
void arc_host_set_instr_hook(void (*hook)(arc_machine_t *m, void *user), void *user);
/* Bus hook, fired only for the access kinds in `watch` (ARC_WATCH_*). */
void arc_host_set_bus_hook(void (*hook)(arc_machine_t *m, void *user, int kind,
                                        uint16_t addr, uint8_t data),
                           void *user, uint8_t watch);
/* Publish the machine's current framebuffer now (the debugger shows the
 * picture being drawn while stopped). Machine thread only. */
void arc_host_publish_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* ARC_HOST_H */
