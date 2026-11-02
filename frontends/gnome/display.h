/*
 * The emulator display: a GtkWidget that pulls frames from the session on
 * the compositor's own frame clock.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <adwaita.h>

#include "arcsession.h"

#define ARC_TYPE_DISPLAY (arc_display_get_type())
G_DECLARE_FINAL_TYPE(ArcDisplay, arc_display, ARC, DISPLAY, GtkWidget)

GtkWidget *arc_display_new(arcsession *session);
/* The picture a television showed (the whole 360 x 243 area at 4:3), or
 * square pixels (360:243). */
void arc_display_set_tv_aspect(ArcDisplay *self, gboolean tv);
void arc_display_set_smooth(ArcDisplay *self, gboolean smooth);
