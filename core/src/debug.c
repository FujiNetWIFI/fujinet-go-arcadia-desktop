/*
 * debug.c -- the session's side of the debugger: one engine per session
 * (core/debugger/debugger.c), told when a power cycle takes the machine
 * away and brings a new one up, and fed FujiNet's traffic from the start.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "session_internal.h"
#include "debugger_internal.h"

arcdebug *arcdebug_get(arcsession *s)
{
    if (!s)
        return NULL;
    if (!s->debugger)
        s->debugger = arcdebug_create();
    return s->debugger;
}

/* The FujiNet trace is always on: it is cheap, and it is most useful about
 * what happened before anyone thought to look. */
void arcdebug_trace_fujinet(struct arcsession *s)
{
    arcdebug_fujinet_install(arcdebug_get(s));
}

void arcdebug_before_power_cycle(struct arcsession *s)
{
    if (s->debugger)
        arcdebug_power_down(s->debugger);
}

void arcdebug_after_power_cycle(struct arcsession *s)
{
    if (!s->debugger)
        return;
    arcdebug_set_cart_path(s->debugger, s->cart_path);
    arcdebug_power_up(s->debugger);
}

void arcdebug_destroy(struct arcsession *s)
{
    arcdebug_free(s->debugger);
    s->debugger = NULL;
}
