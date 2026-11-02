/*
 * The Controllers panel (F9 / Cmd+J): the Arcadia 2001's membrane keyboard and its
 * two joysticks drawn like the console's, the gamepads with their
 * assignments, and Map mode for rebinding. Hidden at start; a fixed-size
 * floating NSPanel.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#import <Cocoa/Cocoa.h>

#include "arcsession.h"

/* The keys the app itself owns (F9, F11, F12): called with the keysym of one
 * typed with the panel key; returns YES when it took it. */
typedef BOOL (^ArcAppKeyHandler)(uint32_t keysym);

@interface ArcControllersWindow : NSObject
- (instancetype)initWithSession:(arcsession *)session appKeys:(ArcAppKeyHandler)appKeys;
/* Show, or hide when showing. */
- (void)toggle;
- (void)hide;
- (BOOL)isVisible;
/* The machine changed (Preferences): the keyboard is rebuilt for it. */
- (void)refresh;
@end
