/*
 * Small views and builders the debugger's tabs share: a picture view with
 * nearest-neighbour scaling and a pointer, a text view that reports clicks on
 * its lines, and the stack and label helpers the tabs are laid out with.
 *
 * Written to the family's AppKit pattern; built and verified on CI only (no
 * Apple toolchain here).
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#import <Cocoa/Cocoa.h>

#include <stdint.h>

/* An image of XRGB pixels (copied), +1 retained; NULL when the size is empty. */
CGImageRef ArcNewImage(const uint32_t *px, int w, int h);

/* A monospaced, read-only text view in its own scroll view; wrap: wraps at the
 * view's width instead of scrolling sideways. */
NSTextView *ArcMonoView(NSScrollView **scrollOut, BOOL wrap);
/* Replace a text view's text, keeping the reader's place. */
void ArcSetTextKeepingPlace(NSTextView *view, NSString *text);
/* Whether a view's reader is at its end (a log then follows new lines). */
BOOL ArcTextAtEnd(NSTextView *view);
/* A log's text where its reader left it, or, following, at its end. */
void ArcSetLogText(NSTextView *view, NSString *text, BOOL follow);
/* C text without its control characters (but for newline and tab). */
NSString *ArcStripControl(const char *s);

NSStackView *ArcHStack(NSArray<NSView *> *views);
NSStackView *ArcVStack(NSArray<NSView *> *views);
/* The view as wide as the stack. */
void ArcFill(NSStackView *stack, NSView *view);
/* A drawn view at its own intrinsic size. */
void ArcFixSize(NSView *view);

NSTextField *ArcLabel(NSString *text);
/* Secondary-coloured small text. */
NSTextField *ArcNote(NSString *text);
/* Bold section title. */
NSTextField *ArcTitle(NSString *text);
NSTextField *ArcField(NSString *placeholder, CGFloat width, id target, SEL action);
NSButton *ArcButton(NSString *title, id target, SEL action);

/* A picture at the largest whole multiple that fits (nearest neighbour, so
 * every pixel of these small pictures is the same size), letterboxed. The
 * pointer is reported in the picture's own pixels. */
@interface ArcPictureView : NSView
/* Takes ownership of the +1 image. */
- (void)setImage:(CGImageRef)image;
/* An outline in picture pixels (x, y, w, h from the top left); empty: none. */
@property (nonatomic) NSRect highlight;
/* The picture pixel under the pointer, or -1, -1 when it is outside. */
@property (nonatomic, copy) void (^onHover)(int x, int y);
@end

/* A text view whose lines can be clicked, and whose wheel can be taken over. */
@interface ArcLineTextView : NSTextView
@property (nonatomic, copy) void (^onClickLine)(int line);
@property (nonatomic, copy) void (^onScroll)(int lines);
@end
