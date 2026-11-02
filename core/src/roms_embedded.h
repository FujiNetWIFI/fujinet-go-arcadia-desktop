/*
 * roms_embedded -- the table tools/roms/embed-roms.py generates into the
 * build tree: empty (the shipping configuration, WITH_ARC_ROMS=OFF) or the
 * recognised images a developer put in tools/roms/ (NOT redistributable;
 * see COMPLIANCE.md).
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARC_ROMS_EMBEDDED_H
#define ARC_ROMS_EMBEDDED_H

#include <stdint.h>

typedef struct {
    const char *file;          /* MAME's file name: "arcbios.rom" */
    const uint8_t *data;
    uint32_t size;
} arc_embedded_rom;

extern const arc_embedded_rom arc_embedded_roms[];
extern const unsigned arc_embedded_rom_count;

#endif /* ARC_ROMS_EMBEDDED_H */
