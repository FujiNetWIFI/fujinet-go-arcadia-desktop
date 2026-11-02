/*
 * The AppKit debugger window over the Signetics 2650 / 2637 UVI debugger engine
 * (arcdebug.h).
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#import <Cocoa/Cocoa.h>

#include "arcsession.h"

@interface ArcDebuggerWindow : NSObject <NSWindowDelegate, NSTextFieldDelegate>
/* Shows (creating on first use) the debugger for the session; attaching the
 * engine stops the machine, as the other frontends do. */
+ (void)showForSession:(arcsession *)session;
/* F12: shows it, or closes it (detaching, so the machine runs on). */
+ (void)toggleForSession:(arcsession *)session;
@end
