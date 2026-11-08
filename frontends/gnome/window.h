/*
 * The main application window.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <adwaita.h>

#include "arcsession.h"

#define ARC_TYPE_WINDOW (arc_window_get_type())
G_DECLARE_FINAL_TYPE(ArcWindow, arc_window, ARC, WINDOW,
                     AdwApplicationWindow)

GtkWidget *arc_window_new(AdwApplication *app, arcsession *session);
/* main.c: the icon name to use (the installed id, or the in-tree art). */
const char *arc_icon_name(void);
void arc_window_toast(ArcWindow *self, const char *text);
/* A message the user has to read (a toast would vanish before the reason
 * was read): an alert with an OK button. */
void arc_window_message(ArcWindow *self, const char *heading, const char *body);
/* The display's shape: 0 = the television's 4:3, 1 = square pixels. */
void arc_window_apply_aspect(ArcWindow *self, int aspect);
/* Smooth (bilinear) scaling of the picture, or the sharp pixels. */
void arc_window_apply_smooth(ArcWindow *self, gboolean smooth);
/* A dropped or command-line file: a cartridge is
 * opened, anything else copied to FujiNet's SD folder. */
void arc_window_load_media(ArcWindow *self, const char *path);
/* A system action (ARC_SYSACT_*) from any window's keys, with the main
 * window's feedback: Reset to CONFIG toasts, Debugger Stop opens the
 * debugger. */
void arc_window_run_sysaction(ArcWindow *self, int sysact);
/* A key from any of the app's windows, handled exactly as the main window
 * handles its own: F9/F11/F12, the system actions, then the machine's keys.
 * Returns whether it was used. keycode is the GDK hardware keycode. */
gboolean arc_window_key(ArcWindow *self, guint keyval, guint keycode,
                       GdkModifierType state, gboolean down);
/* Focus left one of the app's windows: let go of every key the machine
 * holds (and forget which system-action keys were down). */
void arc_window_release_keys(ArcWindow *self);

/* The accent colour (ARCSESSION_ACCENT_RGB, the keypad gold) as
 * CSS classes every window can use: ".arc-accent" paints a widget's
 * background in it (with white text), ".arc-accent-text" its label (a tint of
 * it on a dark theme, the darker red on a light one), and ".arc-pill" makes a
 * label a small rounded indicator. Installed once by the main window. */
void arc_install_accent_css(void);

/* The Help context the main window shows now: "main", or "prefs/<page>"
 * while Preferences is open. */
const char *arc_window_help_context(ArcWindow *self);
