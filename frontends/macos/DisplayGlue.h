/*
 * DisplayGlue.h -- what DisplayView.m needs to know about THIS machine: the
 * session's functions, the frame size and the shape of the picture.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "arcsession.h"

#define FN_VIEW      ArcDisplayView
#define FN_SESSION   arcsession
#define FN_FB_W      ARCSESSION_FB_WIDTH
#define FN_FB_H      ARCSESSION_FB_HEIGHT
#define FN_INITIAL_W 699
#define FN_INITIAL_H 524

/* The television's shape: 4:3 (setting "aspect" 0, the default). */
#define TV_ASPECT (4.0 / 3.0)

static inline int fn_machine_hz(FN_SESSION *s) { return arcsession_refresh_rate(s); }

static inline void fn_notify_vsync(FN_SESSION *s, int64_t ns)
{
    arcsession_notify_vsync(s, ns);
}

/* Every frame is the whole 192 x 262; nothing to measure. */
static inline int fn_pull_frame(FN_SESSION *s, uint32_t *dst, int *lines, uint64_t *serial)
{
    *lines = FN_FB_H;
    return arcsession_copy_frame(s, dst, serial);
}

/* Letterboxed at the TV's 4:3, or at the picture's own 192:262. */
static inline double fn_picture_aspect(FN_SESSION *s, int lines, BOOL tv)
{
    (void)s;
    return tv ? TV_ASPECT : (double)FN_FB_W / (double)lines;
}
