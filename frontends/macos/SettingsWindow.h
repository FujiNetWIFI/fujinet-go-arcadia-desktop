/*
 * The Settings window: Machine, ROMs, Display, Audio, Input and FujiNet pages,
 * the same settings (keys and defaults) as the other frontends' Preferences.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#import <Cocoa/Cocoa.h>

#include "arcsession.h"

/* What the window needs the app to do for it. */
@protocol ArcSettingsDelegate <NSObject>
/* Live display changes (the app's menu and view follow). */
- (void)settingsSetAspect:(int)aspect;
- (void)settingsSetSmooth:(BOOL)smooth;
/* The ROMs page's buttons: pick files (folder NO) or a folder (YES) and import. */
- (void)settingsImportRoms:(BOOL)folder;
- (void)settingsOpenWebUI;
/* The window closed. needsRestart: a machine or host option changed, so the
 * console is power-cycled (arcsession_restart) and a toast says so. */
- (void)settingsClosed:(BOOL)needsRestart;
@end

@interface ArcSettingsWindow : NSObject <NSWindowDelegate>
- (instancetype)initWithSession:(arcsession *)session delegate:(id<ArcSettingsDelegate>)delegate;
- (void)show;
/* The ROM list, the machine's ROM status and the Web UI line, from the session
 * (after an import, a restart). */
- (void)refresh;
/* The View menu changed the picture: the popup and the box follow. */
- (void)syncDisplay;
@end
