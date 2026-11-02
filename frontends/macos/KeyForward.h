/*
 * AppKit key events to X11/xkb keysyms, by hardware key code through the
 * session's HID table -- the same physical-key route the other frontends
 * take, so a binding names the key whatever Shift or the layout is doing.
 *
 * Written to the family's AppKit pattern; built and verified on CI only (no
 * Apple toolchain here).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#import <Cocoa/Cocoa.h>
#include <stdint.h>

/* Returns the keysym, or 0 when the key has no equivalent worth forwarding. */
uint32_t ArcKeysymFromEvent(NSEvent *event);

/* The keysym for a modifier flag change (flagsChanged), or 0. Sets *down. */
uint32_t ArcKeysymFromFlagsChange(NSEvent *event, int *down);

/* This app's accent colour (ARCSESSION_ACCENT_RGB): the console keypad's gold
 * (#AA916C). For fills and highlights; dark text sits on it. */
NSColor *ArcAccentColor(void);

/* The accent for TEXT and thin strokes: on a light appearance the gold is
 * ARCSESSION_ACCENT_DARK_RGB (#7A6340) instead. */
NSColor *ArcAccentTextColor(void);
