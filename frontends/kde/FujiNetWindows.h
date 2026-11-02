/*
 * The FujiNet console log window and the FujiNet configuration window, and
 * the accent colour helpers every window shares.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

extern "C" {
#include "arcsession.h"
}

/* Shows (raising an existing one) the FujiNet console log window. */
void fujinet_log_show(QWidget *parent, arcsession *session);
/* Opens the FujiNet web UI in the system browser. Not embedded: the web UI's
 * Google and OneDrive Authorize buttons open the provider's consent page in a
 * new tab, and both providers refuse OAuth from an embedded web view. */
void fujinet_config_show(QWidget *parent, arcsession *session);

/* The accent colour: the Arcadia 2001's power-button red. It carries WHITE text
 * (a held key, the debugger's current line, the Map target). */
QColor arcAccentColor();
/* Its darker variant, the console's reset key: outlines and text on the
 * light themes, and the RESET cap. */
QColor arcAccentDarkColor();
/* The accent for TEXT (and text-like marks: the status dot, a breakpoint
 * dot): the red on a dark theme, the darker variant on a light one, where
 * the red is too strong to read as a mark. */
QColor arcAccentTextColor(const QWidget *w);
/* A style sheet that fills a button (or a label) in the accent with white
 * text. */
QString arcAccentFill();
