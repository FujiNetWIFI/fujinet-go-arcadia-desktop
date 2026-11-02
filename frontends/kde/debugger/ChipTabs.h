/*
 * The debugger's chip tabs: Sound & I/O (the UVI's beeper, the keypads, panel,
 * discs and pots live) and Cart (the cartridge's mapper or the
 * FujiNet cartridge's state and mailbox page).
 *
 * Each is refreshed by the debugger window while it shows.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QWidget>

#include "arcdebug.h"

class KeyMatrixWidget;
class QLabel;
class QPlainTextEdit;
class StickDiagram;

class IoTab : public QWidget {
public:
    IoTab(arcdebug *debugger, arcsession *session, QWidget *parent = nullptr);
    void refresh();

private:
    arcdebug *m_dbg;
    arcsession *m_session;
    QPlainTextEdit *m_text = nullptr;
    KeyMatrixWidget *m_matrix = nullptr;
    StickDiagram *m_stick[2] = {};
    QLabel *m_reset = nullptr;
};

class CartTab : public QWidget {
public:
    explicit CartTab(arcdebug *debugger, QWidget *parent = nullptr);
    void refresh();

private:
    arcdebug *m_dbg;
    QPlainTextEdit *m_text = nullptr;
    QPlainTextEdit *m_mailbox = nullptr;
};
