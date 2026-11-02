/*
 * The Preferences window -- see prefs.c.
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef ARC_WIN_PREFS_H
#define ARC_WIN_PREFS_H

#include "win_common.h"
#include "arcsession.h"

/* What the window tells the main window. */
typedef struct {
    /* the picture options changed (applied at once) */
    void (*display_changed)(void);
    /* the window closed; dirty = a setting that needs a power cycle changed */
    void (*closed)(int dirty);
} arc_prefs_hooks;

/* Shows the window, or raises it when it is open. */
void arc_prefs_show(HWND parent, arcsession *session, const arc_prefs_hooks *hooks);
/* Closing without applying anything (quitting): the settings are saved. */
void arc_prefs_abandon(void);
/* The gamepad set changed. */
void arc_prefs_gamepads_changed(void);
/* Tab and the arrow keys move between its controls; 1 if the message was used. */
int arc_prefs_pretranslate(MSG *msg);

#endif
