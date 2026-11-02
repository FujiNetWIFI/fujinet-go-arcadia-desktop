/*
 * Debugger window for the GNOME frontend.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <adwaita.h>

#include "arcsession.h"

G_BEGIN_DECLS

/* Shows (creating on first use) the debugger window for the session.
 * Showing attaches the debugger engine, which stops the machine. */
void arc_debugger_show(GtkWindow *parent, arcsession *session);
/* F12: shows it, or hides it (detaching, so the machine runs on). */
void arc_debugger_toggle(GtkWindow *parent, arcsession *session);

G_END_DECLS
