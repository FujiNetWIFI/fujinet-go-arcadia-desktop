/*
 * debugger_internal -- the engine's side of the session glue, the prompt's
 * view of the engine, and what debugger.c shares with uviview.c. Not
 * installed.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARC_DEBUGGER_INTERNAL_H
#define ARC_DEBUGGER_INTERNAL_H

#include "arcdebug.h"
#include "host.h"
#include "arc_internal.h"
#include "symbols.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An engine for the running host, with no session: what arcdebug_get() makes
 * for a session, and what the tests make directly. */
arcdebug *arcdebug_create(void);
void arcdebug_free(arcdebug *d);
/* The machine is about to go away (a power cycle or a stop): resume and
 * unhook, remembering whether a window held the engine. */
void arcdebug_power_down(arcdebug *d);
/* A new machine is up: hook it again if a window held the engine (which
 * stops it at its first instruction, so a power cycle can be debugged from
 * the reset vector). */
void arcdebug_power_up(arcdebug *d);
/* The opened cartridge, for symbols next to it ("" for CONFIG). */
void arcdebug_set_cart_path(arcdebug *d, const char *path);
/* Feed the FujiNet trace from the transport, from now on. */
void arcdebug_fujinet_install(arcdebug *d);

/* For the prompt (prompt.c). */
symtab *arcdebug_symbols(arcdebug *d);
/* Evaluate an expression in the machine's context; 0 or -1 with err. */
int arcdebug_eval(arcdebug *d, const char *text, long *out, char *err, int errsz);
/* Step n instructions (the prompt's `step n`). */
void arcdebug_step_n(arcdebug *d, int n);
/* Register names the evaluator knows, for completion. */
extern const char *const arcdebug_reg_names[];

/* The running machine, or NULL when the host is not running. Every use is
 * under arc_host_lock (or on the machine thread). */
arc_machine_t *arcdebug_machine(void);
/* Where the beam is: the UVI's line and the pixel in it. */
void arcdebug_beam(arc_machine_t *m, int *vpos, int *hpos);

/* uviview.c: the prompt's views of the chip. Each takes the host lock. */
int arcdebug_text_uvi(char *dst, int dstsz);
int arcdebug_text_objects(char *dst, int dstsz);
int arcdebug_text_screen(char *dst, int dstsz);

#ifdef __cplusplus
}
#endif

#endif /* ARC_DEBUGGER_INTERNAL_H */
