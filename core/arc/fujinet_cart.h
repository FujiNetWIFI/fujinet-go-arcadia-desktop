/* fujinet_cart.h -- the FujiNet Arcadia 2001 cartridge, the one cart with a
 * network behind it.
 *
 * A C transposition of the firmware's own MAME device model (fujinet-firmware
 * pico/arcadia/emu/fujinet.cpp, BSD-3-Clause, Thomas Cherryhomes): an RP2040
 * on the cartridge connector (A0-A13, D0-D7, nothing else) that serves the
 * console's reads from RAM and decodes the mailbox hotspots the console READS
 * at image offsets $1D00-$1FFF (console $2D00-$2FFF; $4000/$6000 alias, $3000/
 * $5000/$7000 are open bus). The image planner (arcmap.c), the APPBANK serving
 * (arcapp.h), the mailbox protocol (fujimail.c) and the wire codec (fujibus.c)
 * are the cartridge firmware's own sources, staged verbatim into
 * fuji-generated/ by cmake/StageFujiProto.cmake; this device is only the port.
 *
 * It sits in the cartridge slot through the arc_cart_t interface (carts.h). What
 * it serves at power-on is the baked CONFIG, or an opened FujiNet app; the
 * image the client mounts is staged beside it and swapped in when the client
 * reads $2DFE, armed.
 *
 * Like the cartridge, the mailbox service runs on its own thread (the RP2040's
 * core0) while the bus is served on the emulation thread (core1), so a slow
 * network transaction never stalls the picture; a synchronous mode
 * (FUJINET_ASYNC=0, or the sync flag) runs it inline instead, which is
 * deterministic for tests and is what MAME does.
 *
 * Unlike MAME's device it reconnects: a link that drops (FujiNet restarted, a
 * transaction cut short) is dialled again before the next transaction.
 *
 * Single instance per process: fujimail's port interface is C function
 * pointers with no context argument (the constraint is real hardware's too --
 * one cart slot, one cart).
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARC_FUJINET_CART_H
#define ARC_FUJINET_CART_H

#include <stdbool.h>
#include <stdint.h>

#include "carts.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arc_fujinet arc_fujinet_t;

/* Create the (single) cart; NULL if one already exists. */
arc_fujinet_t *arc_fujinet_create(void);
void arc_fujinet_destroy(arc_fujinet_t *c);

/* The slot interface (valid until destroy). */
arc_cart_t *arc_fujinet_cart(arc_fujinet_t *c);

/* The cart's time, for the 60 Hz tick it paints at $2C0E/F: the machine hands
 * it the CPU's cycle counter (read on the emulation thread) and the CPU clock. */
void arc_fujinet_set_clock(arc_fujinet_t *c, const uint64_t *cycles, double cycle_hz);

/* Power on, as MAME's device_reset with the image it was given:
 *   - `image` NULL: the baked CONFIG client (fujiconfigrom.h), mailbox live;
 *   - an image: served in its place; the mailbox is live when it carries the
 *     "FUJI" claim, dead otherwise (a plain game in the FujiNet slot).
 * `boip` is fujinet-pc's BoIP listener ("host:port"), or "" for none at all
 * (FujiNet switched off: nothing is dialled, every transaction fails at once).
 * Returns 0, or -1 with the reason (an image the cartridge cannot plan) in
 * `why`. */
int arc_fujinet_power_on(arc_fujinet_t *c, const uint8_t *image, uint32_t len,
                         const char *boip, bool sync, char *why, int whysz);
/* Power off: stop the mailbox worker, close the link. */
void arc_fujinet_power_off(arc_fujinet_t *c);

/* Bring what the mailbox worker published into the slot (also done on every
 * fetch); the host calls it once a frame. */
void arc_fujinet_frame(arc_fujinet_t *c);

/* ---- status, for the session and the debugger ---- */
typedef struct {
    bool powered;
    bool mailbox;             /* the running image owns the mailbox pages */
    bool link_up;             /* the BoIP socket is connected */
    bool link_off;            /* FujiNet is switched off: nothing is dialled */
    bool app_live;            /* the running image is an APPBANK one */
    bool booted_game;         /* anything other than the baked CONFIG has been served */
    bool armed;               /* the client armed the swap: the next $2DFE read takes it */
    bool staged_app;          /* a staged image exists and it is an APPBANK one */
    bool staged_mailbox;      /* ... and it claims the mailbox */
    uint32_t live_size, live_crc;
    uint32_t staged_size, staged_crc;
    unsigned swaps;           /* images swapped in since power-on */
    unsigned nbanks;          /* APPBANK: pages incl. page 0; flat: 0 */
    unsigned bank;            /* APPBANK: the page $0800 shows */
    int boot_state, boot_pct, boot_err;
    uint8_t ackseq, status, err, reply_cmd;
    uint16_t rxlen;
    unsigned queue;           /* hotspot reads waiting for the worker */
} arc_cart_status_t;

void arc_fujinet_status(arc_fujinet_t *c, arc_cart_status_t *out);

/* The mailbox pages ($2B00-$2FFF: the reply slice, the status page, the hotspot
 * pages) as the console would read them, with no side effects (the debugger's
 * Cart tab). */
#define ARC_FJ_MAILBOX_BYTES 0x500
void arc_fujinet_mailbox(arc_fujinet_t *c, uint8_t out[ARC_FJ_MAILBOX_BYTES]);

/* The running image's byte at an image offset (0 .. ARCMAP_STORE-1), no side
 * effects (the debugger's CART space). Returns 0xFF past the image. */
uint8_t arc_fujinet_image_byte(arc_fujinet_t *c, uint32_t offset);
/* ... and an edit of it: the running image's own buffer, never a hotspot. */
void arc_fujinet_image_poke(arc_fujinet_t *c, uint32_t offset, uint8_t data);

/* The image served at power-on or by the last swap, as the debugger's Save
 * wants it. Returns bytes copied. */
uint32_t arc_fujinet_copy_image(arc_fujinet_t *c, uint8_t *dst, uint32_t max);

/* 1 if `image` (len bytes) can run on the cartridge, with *claims set when it
 * carries the claim (a FujiNet app); else 0 and why. */
int arc_fujinet_check_image(const uint8_t *image, uint32_t len, bool *claims,
                            char *why, int whysz);

#ifdef __cplusplus
}
#endif

#endif /* ARC_FUJINET_CART_H */
