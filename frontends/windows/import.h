/*
 * File choosers for cartridges -- see import.c.
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef ARC_WIN_IMPORT_H
#define ARC_WIN_IMPORT_H

#include <windows.h>

#include "arcsession.h"

/* One cartridge (.bin .rom .arc, or a .zip of one) for Open Cartridge; 1 if one was chosen. */
int arc_pick_cart(HWND owner, const char *title, char *path, DWORD pathsz);

/* Asks for cartridge images (several) and copies them onto FujiNet's SD
 * host, then says so in one message box. Returns how many were copied, or
 * -1 when the chooser was cancelled. */
int arc_import_carts(HWND owner, arcsession *session);

#endif
