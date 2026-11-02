/*
 * Preferences dialog for the GNOME frontend.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <adwaita.h>

#include "arcsession.h"

G_BEGIN_DECLS

typedef struct _ArcWindow ArcWindow;

/* Shows the preferences dialog. The analog switch, the volume and the aspect
 * apply live; the BIOS switch and the host options (FujiNet, audio,
 * gamepads) are read when the machine powers up, so the dialog power-cycles
 * the session on close if one of those changed. */
void arc_prefs_show(ArcWindow *parent, arcsession *session,
                    void (*restart)(ArcWindow *parent));

G_END_DECLS
