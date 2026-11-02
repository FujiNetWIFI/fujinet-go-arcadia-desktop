/*
 * uvi_test -- the 2637 UVI on its own (core/arc/uvi.c): no CPU, no cartridge.
 * Registers are written as the CPU would, a frame is drawn by ticking the line
 * renderer, and the picture is read back pixel by pixel. The expected pictures
 * are worked out here from MAME's arcadia_v.cpp (its comments and code); the
 * whole machine is held to MAME frame for frame by tools/ab/cart_ab.py and
 * config_frames_test.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>

#include "uvi.h"

static int failures;
static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
}

static uvi_t u;

static void frame(void)
{
    for (int i = 0; i < u.lines; i++)
        uvi_line(&u);
}

static uint32_t px(int x, int y)
{
    return u.bitmap[y * UVI_WIDTH + x];
}

/* the CPU's view: $1800 + offset */
static void w(unsigned off, uint8_t v) { uvi_write(&u, off, v); }

enum { WHITE, YELLOW, CYAN, GREEN, MAGENTA, RED, BLUE, BLACK };
#define C(n) uvi_colours[n]

static void fresh(void)
{
    uvi_init(&u, 262);
    w(UVI_VPOS, 0xff);            /* ypos = 255 - $FF = 0: the first row on line 0 */
    w(UVI_PAL + 1, 0x80 | (WHITE << 3) | BLUE);   /* 8x8 characters; screen blue, characters white */
    w(UVI_PAL + 2, 0xc0);
    w(UVI_PAL + 3, 0xc0);
}

int main(void)
{
    uvi_snapshot s;

    /* the palette: MAME's arcadia_colors */
    check(C(WHITE) == 0xffffff && C(YELLOW) == 0xffff00 && C(CYAN) == 0x00ffff && C(GREEN) == 0x00ff00 &&
          C(MAGENTA) == 0xff00ff && C(RED) == 0xff0000 && C(BLUE) == 0x0000ff && C(BLACK) == 0,
          "the eight colours are white yellow cyan green magenta red blue black");

    /* the background and one character */
    fresh();
    w(UVI_CHARS1, 0x1a);          /* 'A' */
    frame();
    check(px(0, 0) == C(BLUE) && px(191, 261) == C(BLUE), "the screen colour fills the whole picture, border included");
    /* 'A' row 1 = $08: bit 3 -> pixel 4 of 8 (msb is the left edge) */
    check(px(UVI_XPOS + 4, 1) == C(WHITE) && px(UVI_XPOS + 3, 1) == C(BLUE) && px(UVI_XPOS + 5, 1) == C(BLUE),
          "character $1A is an A: row 1 has one lit pixel, the 5th");
    check(px(UVI_XPOS + 2, 5) == C(WHITE) && px(UVI_XPOS + 6, 5) == C(WHITE) && px(UVI_XPOS + 4, 5) == C(WHITE),
          "and row 5 is the crossbar");
    check(px(UVI_XPOS, 0) == C(BLUE) && px(UVI_XPOS + 3, 8) == C(BLUE), "row 0 and the line below the character are blank");
    check(uvi_read(&u, UVI_CHARS1) == 0x1a, "the screen RAM reads back");

    /* colour bits: cc = (pal1 >> 3 & 1) | (code >> 5 & 6) */
    fresh();
    w(UVI_PAL + 1, 0x80 | (YELLOW << 3) | BLUE);   /* chars: bit 0 set (yellow = 1) */
    w(UVI_CHARS1, 0x03);          /* solid block, colour bits 00: cc = 1 = yellow */
    w(UVI_CHARS1 + 1, 0x43);      /* bit 6: cc = 1 | 2 = 3 = green */
    w(UVI_CHARS1 + 2, 0x83);      /* bit 7: cc = 1 | 4 = 5 = red */
    w(UVI_CHARS1 + 3, 0xc3);      /* both: cc = 7 = black */
    frame();
    check(px(UVI_XPOS + 3, 3) == C(YELLOW) && px(UVI_XPOS + 11, 3) == C(GREEN) &&
          px(UVI_XPOS + 19, 3) == C(RED) && px(UVI_XPOS + 27, 3) == C(BLACK),
          "the two high bits of a character select its colour (single colour mode)");

    /* multicolour: $18FD bit 7 -> the colour comes whole from the palette bytes */
    fresh();
    w(UVI_PAL, (MAGENTA << 3) | CYAN);               /* alt: chars magenta, screen cyan */
    w(UVI_SOUND1, 0x80);
    w(UVI_CHARS1, 0x03);          /* bit 6 and 7 clear: chars from pal0 */
    w(UVI_CHARS1 + 1, 0xc3);      /* both set: chars/screen from pal1 */
    frame();
    check(px(UVI_XPOS + 3, 3) == C(MAGENTA), "multicolour: a clear colour bit takes $19F8's character colour");
    check(px(UVI_XPOS + 11, 3) == C(WHITE), "and a set one $19F9's");

    /* user defined characters */
    fresh();
    w(UVI_UDC + 4 * 8 + 0, 0x81);   /* UDC $3C */
    w(UVI_UDC + 4 * 8 + 7, 0xff);
    w(UVI_CHARS1, 0x3c);
    frame();
    check(px(UVI_XPOS, 0) == C(WHITE) && px(UVI_XPOS + 7, 0) == C(WHITE) && px(UVI_XPOS + 3, 0) == C(BLUE) &&
          px(UVI_XPOS + 3, 7) == C(WHITE), "$1980-$19BF are the user defined characters $38-$3F");

    /* 8x16 characters: pal1 bit 7 clear doubles every row */
    fresh();
    w(UVI_PAL + 1, (WHITE << 3) | BLUE);
    w(UVI_CHARS1, 0x1a);
    frame();
    check(px(UVI_XPOS + 4, 2) == C(WHITE) && px(UVI_XPOS + 4, 3) == C(WHITE) && px(UVI_XPOS + 4, 1) == C(BLUE),
          "with the size bit low a character row is 16 lines: each glyph row is drawn twice");
    check(px(UVI_XPOS + 2, 10) == C(WHITE) && px(UVI_XPOS + 2, 11) == C(WHITE), "crossbar on lines 10-11");

    /* thirteen rows, or twenty-six in refresh mode */
    fresh();
    w(UVI_CHARS1 + 12 * 16, 0x03);       /* the last row of the upper screen */
    w(UVI_CHARS2, 0x03);                 /* the first of the lower: hidden unless $19F8 bit 6 */
    frame();
    check(px(UVI_XPOS + 2, 12 * 8 + 2) == C(WHITE) && px(UVI_XPOS + 2, 13 * 8 + 2) == C(BLUE),
          "13 rows are shown; the lower screen RAM is not");
    w(UVI_PAL, 0x40);
    frame();
    check(px(UVI_XPOS + 2, 13 * 8 + 2) == C(WHITE), "with $19F8 bit 6 the lower 13 rows follow");

    /* the first character line moves with $18FC, the scroll with $18FE */
    fresh();
    w(UVI_VPOS, 0xff - 20);              /* ypos = 255 - $EB = 20 */
    w(UVI_CHARS1, 0x03);
    frame();
    check(px(UVI_XPOS + 2, 19) == C(BLUE) && px(UVI_XPOS + 2, 20) == C(WHITE), "$18FC sets the first line of the character area");
    w(UVI_SOUND2, 3 << 5);               /* scroll 3 pixels */
    frame();
    check(px(UVI_XPOS + 3, 20) == C(WHITE) && px(UVI_XPOS + 2, 20) == C(BLUE), "the high 3 bits of $18FE scroll the row");
    uvi_snapshot_get(&u, &s);
    check(s.ypos == 20 && s.shift == 3, "and the snapshot says so");

    /* block graphics: $C0 switches a row to 3x2 blocks, $40 back */
    fresh();
    w(UVI_CHARS1, 0xc0);                 /* graphics on, a blank block */
    w(UVI_CHARS1 + 1, 0x01);             /* bit 0: the two rightmost pixels of the top half */
    w(UVI_CHARS1 + 2, 0x40);             /* graphics off from here */
    w(UVI_CHARS1 + 3, 0x03);
    frame();
    check(px(UVI_XPOS + 8 + 6, 0) == C(WHITE) && px(UVI_XPOS + 8 + 7, 3) == C(WHITE) && px(UVI_XPOS + 8 + 5, 0) == C(BLUE) &&
          px(UVI_XPOS + 8 + 6, 4) == C(BLUE),
          "block graphics: bit 0 of the code is the top half's two rightmost pixels (3x2-pixel blocks)");
    check(px(UVI_XPOS + 24 + 3, 3) == C(WHITE), "and the row is back to characters after $40");

    /* objects: position, colour, shape, the size bit, the draw on the last line */
    fresh();
    w(UVI_UDC, 0xff);                    /* object 0's image, all rows: */
    for (int k = 1; k < 8; k++)
        w(UVI_UDC + k, 0x80);
    w(UVI_OBJ + 0, 0xff - 99);           /* y = ((v ^ $FF) + 1) = 100 */
    w(UVI_OBJ + 1, 43 + 20);             /* x = v - 43 = 20 */
    w(UVI_PAL + 3, 0x40 | (RED << 3));   /* object 0 red, 8x16 (its size bit low); object 1 white on 8x8 */
    frame();
    check(px(UVI_XPOS + 20, 100) == C(RED) && px(UVI_XPOS + 27, 100) == C(RED) && px(UVI_XPOS + 28, 100) == C(BLUE),
          "an object is 8 pixels wide at x = register - 43");
    check(px(UVI_XPOS + 27, 101) == C(RED) && px(UVI_XPOS + 21, 103) == C(BLUE) && px(UVI_XPOS + 20, 103) == C(RED) &&
          px(UVI_XPOS + 20, 115) == C(RED) && px(UVI_XPOS + 20, 116) == C(BLUE),
          "with the size bit low the object is 8x16: rows doubled");
    uvi_snapshot_get(&u, &s);
    check(s.obj[0].colour == RED && s.obj[0].double_height && s.obj[0].x == 20 && s.obj[0].y == 100 &&
          s.obj[0].visible, "the snapshot decodes object 0");
    w(UVI_PAL + 3, 0x80 | 0x40 | (RED << 3));
    frame();
    check(px(UVI_XPOS + 27, 100) == C(RED) && px(UVI_XPOS + 21, 101) == C(BLUE) && px(UVI_XPOS + 20, 107) == C(RED) &&
          px(UVI_XPOS + 20, 108) == C(BLUE),
          "size bit high: 8x8");

    /* collisions: low = hit, set at the start of each frame's sprite draw */
    fresh();
    w(UVI_UDC, 0xff);                    /* object 0 */
    w(UVI_UDC + 8, 0xff);                /* object 1 overlaps object 0 */
    w(UVI_OBJ + 0, 0xff - 49);           /* y 50 */
    w(UVI_OBJ + 1, 43 + 8);              /* x 8 */
    w(UVI_OBJ + 2, 0xff - 49);
    w(UVI_OBJ + 3, 43 + 12);             /* x 12: 4 pixels of overlap */
    w(UVI_PAL + 3, 0xc0 | (RED << 3) | YELLOW);
    frame();
    check((uvi_read(&u, UVI_COLL_SPR) & 1) == 1 ? 0 : 1, "objects 0 and 1 colliding clears bit 0 of $19FD");
    check((uvi_read(&u, UVI_COLL_SPR) & 0x3e) == 0x3e, "and no other pair is reported");
    check((uvi_read(&u, UVI_COLL_BG) & 0xf) == 0xf, "no object touches a character yet");
    w(UVI_CHARS1 + 6 * 16 + 1, 0x03);    /* a solid block at row 6 (y 48-55), column 1 (x 8-15) */
    frame();
    check((uvi_read(&u, UVI_COLL_BG) & 3) == 0, "objects over a character clear their bits of $19FC");
    w(UVI_OBJ + 0, 0xff - 149);          /* move object 0 away */
    w(UVI_OBJ + 2, 0xff - 149);
    w(UVI_OBJ + 3, 43 + 100);
    frame();
    check((uvi_read(&u, UVI_COLL_SPR) & 0x3f) == 0x3f && (uvi_read(&u, UVI_COLL_BG) & 0xf) == 0xf,
          "and everything is clear again the next frame");

    /* the beam: line, charline, SENSE */
    fresh();
    check(!uvi_vblank(&u), "SENSE is low at the top");
    for (int i = 0; i < 215; i++)
        uvi_line(&u);
    check(!uvi_vblank(&u) && u.line == 215, "and still at line 215");
    uvi_line(&u);
    check(uvi_vblank(&u), "SENSE is high from line 216");
    check((uvi_read(&u, UVI_CHARLINE) & 0xf0) == 0xf0 && (uvi_read(&u, UVI_CHARLINE) & 0xf) == 0xd,
          "$18FF reads $FD in the vertical blank");
    for (int i = 0; i < 46; i++)
        uvi_line(&u);
    check(u.line == 0 && !uvi_vblank(&u), "a frame is 262 lines");
    for (int i = 0; i < 9; i++)
        uvi_line(&u);
    check((uvi_read(&u, UVI_CHARLINE) & 0xf) == 1, "$18FF counts the character row (row 1 from line 8)");

    /* inputs as the CPU reads them */
    fresh();
    u.in.pad[0] = 0xf0 | 0x0a;
    u.in.panel = 5;
    u.in.pot[0][0] = 10; u.in.pot[0][1] = 20; u.in.pot[1][0] = 30; u.in.pot[1][1] = 40;
    check(uvi_read(&u, 0x100) == 0xfa && uvi_read(&u, 0x108) == 5, "the keypad and panel reads are the host's latched bytes");
    u.ad_select = 0;
    check(uvi_read(&u, UVI_POT2) == 20 && uvi_read(&u, UVI_POT1) == 40, "pot mux low: the y axes ($19FF is player 1)");
    u.ad_select = 0x40;
    check(uvi_read(&u, UVI_POT2) == 10 && uvi_read(&u, UVI_POT1) == 30, "pot mux high: the x axes");
    w(UVI_PAL + 1, 0x40);
    check(u.ad_delay == 10, "writing $19F9 starts the 10-line settle delay");
    u.ad_select = 0;
    for (int i = 0; i < 10; i++)
        uvi_line(&u);
    check(u.ad_select == 0, "the mux does not follow during it");
    uvi_line(&u);
    check(u.ad_select == 0x40, "and does after");

    /* the debugger's views */
    fresh();
    w(UVI_CHARS1, 0x1a);
    w(UVI_CHARS1 + 17, 0x1b);
    w(UVI_UDC, 0xff);
    w(UVI_OBJ, 0xff - 30);
    w(UVI_OBJ + 1, 43 + 50);
    w(UVI_PAL + 3, 0xc0 | (GREEN << 3));
    frame();
    frame();
    {
        static uint32_t re[UVI_WIDTH * UVI_HEIGHT], now[UVI_WIDTH * UVI_HEIGHT];
        char name[48], text[96];

        uvi_debug_render(&u, UVI_LAYER_BG | UVI_LAYER_SPRITES, re);
        check(memcmp(re, u.bitmap, sizeof re) == 0, "rendering from the registers gives the frame the beam drew, for a steady picture");
        uvi_frame_in_progress(&u, now);
        check(memcmp(now, u.bitmap, sizeof now) == 0, "the frame in progress is the bitmap");
        uvi_debug_render(&u, UVI_LAYER_BG, re);
        check(re[30 * UVI_WIDTH + UVI_XPOS + 50] != C(GREEN) && re[3 * UVI_WIDTH + UVI_XPOS + 4] == u.bitmap[3 * UVI_WIDTH + UVI_XPOS + 4],
              "a layer can be switched off");
        uvi_describe_register(&u, UVI_PAL + 1, name, sizeof name, text, sizeof text);
        check(!strcmp(name, "PAL1") && strstr(text, "white"), "registers are named and decoded");
        uvi_describe_register(&u, UVI_OBJ + 1, name, sizeof name, text, sizeof text);
        check(!strcmp(name, "OBJ0_X") && !strcmp(text, "x=50"), "object registers show the position");
        check(uvi_render_font_sheet(&u, 2, 0xffffff, 0, re, 100, &(int){0}, &(int){0}) == -1, "a sheet that does not fit says so");
    }
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
