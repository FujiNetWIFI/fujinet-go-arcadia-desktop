/*
 * The FujiNet console log window -- see fujilog.c.
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef ARC_WIN_FUJILOG_H
#define ARC_WIN_FUJILOG_H

#include "win_common.h"
#include "arcsession.h"

/* Shows (raising an existing one) the FujiNet console log window: the
 * in-process runtime's captured output, refreshed once a second while the
 * window is open. */
void arc_fujilog_show(HWND parent, arcsession *session);

#endif
