/*
 * The application delegate: the window, the menu bar, and the session.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#import <Cocoa/Cocoa.h>

#include "arcsession.h"

@interface ArcAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
/* mediaPath: a file named on the command line (a cartridge or
 * anything for the SD folder), or NULL. */
- (instancetype)initWithSession:(arcsession *)session mediaPath:(const char *)mediaPath;
@end
