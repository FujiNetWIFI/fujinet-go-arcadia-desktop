/*
 * File and folder choosers for cartridges and the BIOS -- see import.c.
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef ARC_WIN_IMPORT_H
#define ARC_WIN_IMPORT_H

#include <windows.h>

#include "arcsession.h"

/* One cartridge (.bin .rom .arc) for Open Cartridge; 1 if one was chosen. */
int arc_pick_cart(HWND owner, const char *title, char *path, DWORD pathsz);

/* Asks for BIOS files (several, zips included) or a folder, imports each,
 * and says in one message box what came of it. Returns how many ROM items
 * were imported (0: nothing recognised), or -1 when the chooser was
 * cancelled. */
int arc_import_bios(HWND owner, arcsession *session, int folder);

/* Imports dropped or named files and folders the same way, one message box
 * for the lot. Returns how many ROM items came in. */
int arc_import_roms(HWND owner, arcsession *session, const char *const *paths, int n);

/* Asks for cartridge images (several) and copies them onto FujiNet's SD
 * host, then says so in one message box. Returns how many were copied, or
 * -1 when the chooser was cancelled. */
int arc_import_carts(HWND owner, arcsession *session);

#endif
