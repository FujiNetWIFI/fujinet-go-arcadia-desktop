/* uvi.c -- the Signetics 2637 UVI. See uvi.h.
 *
 * Transposed from MAME's src/mame/arcadia/arcadia_v.cpp (GPL-2.0-or-later,
 * copyright-holders Peter Trauner; consolidation by Manfred Schneider, based
 * on work by PeT and Paul Robson) and the palette of arcadia.cpp. The glyph
 * table is MAME's, "read from the screen generated from a palladium": the
 * character ROM has never been dumped.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "uvi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* arcadia_colors[]: white, yellow, cyan, green, magenta, red, blue, black */
const uint32_t uvi_colours[8] = {
    0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00,
    0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00000000,
};

const char *uvi_colour_name(int index)
{
    static const char *const names[8] = {
        "white", "yellow", "cyan", "green", "magenta", "red", "blue", "black",
    };

    return names[index & 7];
}

static const uint8_t font[0x40][8] = {
    { 0,0,0,0,0,0,0,0 },                        /* 00 (space) */
    { 1,2,4,8,16,32,64,128 },                   /* 01 (\) */
    { 128,64,32,16,8,4,2,1 },                   /* 02 (/) */
    { 255,255,255,255,255,255,255,255 },        /* 03 (solid block) */
    { 0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00 },/* 04 (?) */
    { 3,3,3,3,3,3,3,3 },                        /* 05 (half square right on) */
    { 0,0,0,0,0,0,255,255 },                    /* 06 (horz lower line) */
    { 0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0 },/* 07 (half square left on) */
    { 0xff,0xff,3,3,3,3,3,3 },                  /* 08 (?) */
    { 0xff,0xff,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0 },/* 09 (?) */
    { 192,192,192,192,192,192,255,255 },        /* 0A (!_) */
    { 3,3,3,3,3,3,255,255 },                    /* 0B (_!) */
    { 1,3,7,15,31,63,127,255 },                 /* 0C (diagonal block) */
    { 128,192,224,240,248,252,254,255 },        /* 0D (diagonal block) */
    { 255,254,252,248,240,224,192,128 },        /* 0E (diagonal block) */
    { 255,127,63,31,15,7,3,1 },                 /* 0F (diagonal block) */
    { 0x00,0x1c,0x22,0x26,0x2a,0x32,0x22,0x1c },/* 10 0 */
    { 0x00,0x08,0x18,0x08,0x08,0x08,0x08,0x1c },/* 11 1 */
    { 0x00,0x1c,0x22,0x02,0x0c,0x10,0x20,0x3e },/* 12 2 */
    { 0x00,0x3e,0x02,0x04,0x0c,0x02,0x22,0x1c },/* 13 3 */
    { 0x00,0x04,0x0c,0x14,0x24,0x3e,0x04,0x04 },/* 14 4 */
    { 0x00,0x3e,0x20,0x3c,0x02,0x02,0x22,0x1c },/* 15 5 */
    { 0x00,0x0c,0x10,0x20,0x3c,0x22,0x22,0x1c },/* 16 6 */
    { 0x00,0x7c,0x02,0x04,0x08,0x10,0x10,0x10 },/* 17 7 */
    { 0x00,0x1c,0x22,0x22,0x1c,0x22,0x22,0x1c },/* 18 8 */
    { 0x00,0x1c,0x22,0x22,0x3e,0x02,0x04,0x18 },/* 19 9 */
    { 0x00,0x08,0x14,0x22,0x22,0x3e,0x22,0x22 },/* 1A A */
    { 0x00,0x3c,0x22,0x22,0x3c,0x22,0x22,0x3c },/* 1B B */
    { 0x00,0x1c,0x22,0x20,0x20,0x20,0x22,0x1c },/* 1C C */
    { 0x00,0x3c,0x22,0x22,0x22,0x22,0x22,0x3c },/* 1D D */
    { 0x00,0x3e,0x20,0x20,0x3c,0x20,0x20,0x3e },/* 1E E */
    { 0x00,0x3e,0x20,0x20,0x3c,0x20,0x20,0x20 },/* 1F F */
    { 0x00,0x1e,0x20,0x20,0x20,0x26,0x22,0x1e },/* 20 G */
    { 0x00,0x22,0x22,0x22,0x3e,0x22,0x22,0x22 },/* 21 H */
    { 0x00,0x1c,0x08,0x08,0x08,0x08,0x08,0x1c },/* 22 I */
    { 0x00,0x02,0x02,0x02,0x02,0x02,0x22,0x1c },/* 23 J */
    { 0x00,0x22,0x24,0x28,0x30,0x28,0x24,0x22 },/* 24 K */
    { 0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x3e },/* 25 L */
    { 0x00,0x22,0x36,0x2a,0x2a,0x22,0x22,0x22 },/* 26 M */
    { 0x00,0x22,0x22,0x32,0x2a,0x26,0x22,0x22 },/* 27 N */
    { 0x00,0x1c,0x22,0x22,0x22,0x22,0x22,0x1c },/* 28 O */
    { 0x00,0x3c,0x22,0x22,0x3c,0x20,0x20,0x20 },/* 29 P */
    { 0x00,0x1c,0x22,0x22,0x22,0x2a,0x24,0x1a },/* 2A Q */
    { 0x00,0x3c,0x22,0x22,0x3c,0x28,0x24,0x22 },/* 2B R */
    { 0x00,0x1c,0x22,0x20,0x1c,0x02,0x22,0x1c },/* 2C S */
    { 0x00,0x3e,0x08,0x08,0x08,0x08,0x08,0x08 },/* 2D T */
    { 0x00,0x22,0x22,0x22,0x22,0x22,0x22,0x1c },/* 2E U */
    { 0x00,0x22,0x22,0x22,0x22,0x22,0x14,0x08 },/* 2F V */
    { 0x00,0x22,0x22,0x22,0x2a,0x2a,0x36,0x22 },/* 30 W */
    { 0x00,0x22,0x22,0x14,0x08,0x14,0x22,0x22 },/* 31 X */
    { 0x00,0x22,0x22,0x14,0x08,0x08,0x08,0x08 },/* 32 Y */
    { 0x00,0x3e,0x02,0x04,0x08,0x10,0x20,0x3e },/* 33 Z */
    { 0,0,0,0,0,0,0,8 },                        /* 34 . */
    { 0,0,0,0,0,8,8,0x10 },                     /* 35 , */
    { 0,0,8,8,0x3e,8,8,0 },                     /* 36 + */
    { 0,8,0x1e,0x28,0x1c,0xa,0x3c,8 },          /* 37 $ */
    /* 38-3F: the user defined characters, from $1980 */
};

void uvi_init(uvi_t *u, int lines)
{
    memset(u, 0, sizeof *u);
    u->lines = lines;
    memcpy(u->chars, font, sizeof font);
    for (int i = 0; i < 0x40; i++) {
        u->rectangle[i][0] = 0;
        u->rectangle[i][4] = 0;
        if (i & 1)    u->rectangle[i][0] |= 3;
        if (i & 2)    u->rectangle[i][0] |= 0x1c;
        if (i & 4)    u->rectangle[i][0] |= 0xe0;
        if (i & 8)    u->rectangle[i][4] |= 3;
        if (i & 0x10) u->rectangle[i][4] |= 0x1c;
        if (i & 0x20) u->rectangle[i][4] |= 0xe0;
        u->rectangle[i][1] = u->rectangle[i][2] = u->rectangle[i][3] = u->rectangle[i][0];
        u->rectangle[i][5] = u->rectangle[i][6] = u->rectangle[i][7] = u->rectangle[i][4];
    }
    /* the UVI's colour latches idle at white-on-white until written; MAME's
     * bitmap starts as zero pens = white */
    for (size_t i = 0; i < sizeof u->bitmap / sizeof u->bitmap[0]; i++)
        u->bitmap[i] = uvi_colours[0];
    u->in.pad[0] = u->in.pad[1] = u->in.pad[2] = u->in.pad[3] = 0xf0;
    u->in.pad[4] = u->in.pad[5] = u->in.pad[6] = u->in.pad[7] = 0xf0;
    u->in.pot[0][0] = u->in.pot[0][1] = 0x6f;
    u->in.pot[1][0] = u->in.pot[1][1] = 0x80;
}

/* ---- the registers ---- */

uint8_t uvi_read(const uvi_t *u, unsigned offset)
{
    offset &= 0x3ff;
    switch (offset) {
    case 0xff:
        return (uint8_t)(u->charline | 0xf0);
    case 0x100: case 0x101: case 0x102: case 0x103:
    case 0x104: case 0x105: case 0x106: case 0x107:
        return u->in.pad[offset - 0x100];
    case 0x108:
        return u->in.panel;
    case 0x1fe:
        return u->in.pot[1][u->ad_select ? 0 : 1];
    case 0x1ff:
        return u->in.pot[0][u->ad_select ? 0 : 1];
    default:
        return u->reg[offset];
    }
}

void uvi_write(uvi_t *u, unsigned offset, uint8_t data)
{
    offset &= 0x3ff;
    u->reg[offset] = data;
    switch (offset) {
    case 0xfc:
        u->ypos = 255 - data + UVI_YPOS;
        break;
    case 0xfd:
        u->multicolor = data & 0x80;
        break;
    case 0xfe:
        u->shift = data >> 5;
        break;
    case 0xf0: case 0xf2: case 0xf4: case 0xf6:
        u->pos[(offset >> 1) & 3].y = (data ^ 0xff) + 1;
        break;
    case 0xf1: case 0xf3: case 0xf5: case 0xf7:
        u->pos[(offset >> 1) & 3].x = data - 43;
        break;
    case 0x180 ... 0x1bf:
        u->chars[0x38 | ((offset >> 3) & 7)][offset & 7] = data;
        break;
    case 0x1f8:
        u->lines26 = data & 0x40;
        u->graphics = data & 0x80;
        break;
    case 0x1f9:
        u->doublescan = !(data & 0x80);
        u->ad_delay = 10;
        break;
    }
}

/* ---- drawing (arcadia_v.cpp) ---- */

#define PAL(u, n) ((u)->reg[UVI_PAL + (n)])

static void plot_box(uvi_t *u, int y, int h, int colour)
{
    for (int r = y; r < y + h; r++)
        if (r >= 0 && r < UVI_HEIGHT)
            for (int x = 0; x < UVI_WIDTH; x++)
                u->bitmap[r * UVI_WIDTH + x] = uvi_colours[colour];
}

static void draw_char(uvi_t *u, const uint8_t *ch, int charcode, int y, int x)
{
    int k, b, cc, sc;

    if (u->multicolor) {
        cc = (PAL(u, (charcode & 0x40) ? 1 : 0) >> 3) & 7;
        sc = PAL(u, (charcode & 0x80) ? 1 : 0) & 7;
    } else {
        cc = ((PAL(u, 1) >> 3) & 1) | ((charcode >> 5) & 6);
        sc = PAL(u, 1) & 7;
    }

    {
        const uint32_t cbg = uvi_colours[sc], cfg = uvi_colours[cc];
        const int height = UVI_HEIGHT;

        if (u->doublescan) {
            for (k = 0; k < 8 && y < height; k++, y += 2) {
                b = ch[k];
                u->bg[y][x >> 3] |= (uint8_t)(b >> (x & 7));
                u->bg[y][(x >> 3) + 1] |= (uint8_t)(b << (8 - (x & 7)));
                if (y + 1 < height) {
                    u->bg[y + 1][x >> 3] |= (uint8_t)(b >> (x & 7));
                    u->bg[y + 1][(x >> 3) + 1] |= (uint8_t)(b << (8 - (x & 7)));
                    for (int j = 0; j < 8; j++) {
                        const uint32_t c = (b & (0x80 >> j)) ? cfg : cbg;

                        if (x + j < UVI_WIDTH) {
                            u->bitmap[y * UVI_WIDTH + x + j] = c;
                            u->bitmap[(y + 1) * UVI_WIDTH + x + j] = c;
                        }
                    }
                }
            }
        } else {
            for (k = 0; k < 8 && y < height; k++, y++) {
                b = ch[k];
                u->bg[y][x >> 3] |= (uint8_t)(b >> (x & 7));
                u->bg[y][(x >> 3) + 1] |= (uint8_t)(b << (8 - (x & 7)));
                for (int j = 0; j < 8; j++)
                    if (x + j < UVI_WIDTH)
                        u->bitmap[y * UVI_WIDTH + x + j] = (b & (0x80 >> j)) ? cfg : cbg;
            }
        }
    }
}

static void draw_row(uvi_t *u, int y, const uint8_t *chars1)
{
    int x, ch, j, h;
    int graphics = u->graphics;

    h = u->doublescan ? 16 : 8;
    if (UVI_HEIGHT - u->line < h)
        h = UVI_HEIGHT - u->line;
    if (h < 0)
        h = 0;

    plot_box(u, y, h, PAL(u, 1) & 7);
    if (y >= 0 && y + h <= UVI_HEIGHT)
        memset(u->bg[y], 0, sizeof u->bg[0] * (size_t)h);

    for (x = UVI_XPOS + u->shift, j = 0; j < 16; j++, x += 8) {
        ch = chars1[j];
        /* hangman switches with 0x40; alien invaders' shield lines start with 0xc0 */
        if ((ch & 0x3f) == 0) {
            switch (ch) {
            case 0xc0: graphics = 1; break;
            case 0x40: graphics = 0; break;
            }
        }
        if (graphics)
            draw_char(u, u->rectangle[ch & 0x3f], ch, y, x);
        else
            draw_char(u, u->chars[ch & 0x3f], ch, y, x);
    }
}

/* m_bg[y][col] as MAME reads it. A doubled object near the bottom reaches rows
 * past the 262 the array has: MAME (undefined behaviour) reads on into the
 * members declared after it, m_rectangle and then m_chars, and so do we, because
 * those reads decide $19FC. */
static int bg_byte(const uvi_t *u, int y, int col)
{
    size_t flat;

    if (y < UVI_HEIGHT)
        return u->bg[y][col];
    flat = (size_t)(y - UVI_HEIGHT) * UVI_BG_BYTES + (size_t)col;
    if (flat < sizeof u->rectangle)
        return ((const uint8_t *)u->rectangle)[flat];
    flat -= sizeof u->rectangle;
    if (flat < sizeof u->chars)
        return ((const uint8_t *)u->chars)[flat];
    return 0;
}

static int sprite_collision(const uvi_t *u, int n1, int n2)
{
    int k, b1, b2, x;

    if (u->pos[n1].x + 8 <= u->pos[n2].x)
        return 0;
    if (u->pos[n1].x >= u->pos[n2].x + 8)
        return 0;

    for (k = 0; k < 8; k++) {
        if (u->pos[n1].y + k < u->pos[n2].y)
            continue;
        if (u->pos[n1].y + k >= u->pos[n2].y + 8)
            break;
        x = u->pos[n1].x - u->pos[n2].x;
        b1 = u->reg[UVI_UDC + n1 * 8 + k];
        b2 = u->reg[UVI_UDC + n2 * 8 + (u->pos[n1].y + k - u->pos[n2].y)];
        if (x < 0)
            b2 >>= -x;
        if (x > 0)
            b1 >>= x;
        if (b1 & b2)
            return 1;
    }
    return 0;
}

static void draw_sprites(uvi_t *u)
{
    int i, k, x, y = 0, colour = 0;

    u->reg[UVI_COLL_BG] |= 0xf;
    u->reg[UVI_COLL_SPR] |= 0x3f;
    for (i = 0; i < 4; i++) {
        int doublescan = 0;

        if (u->pos[i].y <= -UVI_YPOS) continue;
        if (u->pos[i].y >= UVI_HEIGHT - UVI_YPOS - 8) continue;
        if (u->pos[i].x <= -UVI_XPOS) continue;
        if (u->pos[i].x >= 128 + UVI_XPOS - 8) continue;

        switch (i) {
        case 0: colour = (PAL(u, 3) >> 3) & 7; doublescan = !(PAL(u, 3) & 0x80); break;
        case 1: colour = PAL(u, 3) & 7;        doublescan = !(PAL(u, 3) & 0x40); break;
        case 2: colour = (PAL(u, 2) >> 3) & 7; doublescan = !(PAL(u, 2) & 0x80); break;
        case 3: colour = PAL(u, 2) & 7;        doublescan = !(PAL(u, 2) & 0x40); break;
        }
        for (k = 0; k < 8; k++) {
            int j, m;
            int b = u->reg[UVI_UDC + i * 8 + k];

            x = u->pos[i].x + UVI_XPOS;
            if (!doublescan) {
                y = u->pos[i].y + UVI_YPOS + k;
                for (j = 0, m = 0x80; j < 8; j++, m >>= 1)
                    if ((b & m) && y < UVI_HEIGHT)
                        u->bitmap[y * UVI_WIDTH + x + j] = uvi_colours[colour];
            } else {
                y = u->pos[i].y + UVI_YPOS + k * 2;
                for (j = 0, m = 0x80; j < 8; j++, m >>= 1) {
                    if (b & m) {
                        if (y < UVI_HEIGHT)
                            u->bitmap[y * UVI_WIDTH + x + j] = uvi_colours[colour];
                        if (y + 1 < UVI_HEIGHT)
                            u->bitmap[(y + 1) * UVI_WIDTH + x + j] = uvi_colours[colour];
                    }
                }
            }
            if (u->reg[UVI_COLL_BG] & (1 << i)) {
                if ((b << (8 - (x & 7))) & ((bg_byte(u, y, x >> 3) << 8) | bg_byte(u, y, (x >> 3) + 1)))
                    u->reg[UVI_COLL_BG] &= (uint8_t)~(1 << i);
            }
        }
    }
    if (sprite_collision(u, 0, 1)) u->reg[UVI_COLL_SPR] &= (uint8_t)~1;
    if (sprite_collision(u, 0, 2)) u->reg[UVI_COLL_SPR] &= (uint8_t)~2;
    if (sprite_collision(u, 0, 3)) u->reg[UVI_COLL_SPR] &= (uint8_t)~4;
    if (sprite_collision(u, 1, 2)) u->reg[UVI_COLL_SPR] &= (uint8_t)~8;
    if (sprite_collision(u, 1, 3)) u->reg[UVI_COLL_SPR] &= (uint8_t)~0x10;     /* guess */
    if (sprite_collision(u, 2, 3)) u->reg[UVI_COLL_SPR] &= (uint8_t)~0x20;     /* guess */
}

static void line_impl(uvi_t *u, unsigned layers)
{
    if (u->ad_delay <= 0)
        u->ad_select = PAL(u, 1) & 0x40;
    else
        u->ad_delay--;

    u->line++;
    u->line %= u->lines;
    u->line_count++;

    /* unbelievable, reflects only charline, but alien invaders uses it for
     * alien scrolling */
    if (u->line < u->ypos) {
        plot_box(u, u->line, 1, PAL(u, 1) & 7);
        if (u->line < UVI_HEIGHT)
            memset(u->bg[u->line], 0, sizeof u->bg[0]);
    } else {
        const int h = u->doublescan ? 16 : 8;

        u->charline = (u->line - u->ypos) / h;
        if (u->charline < 13) {
            if (((u->line - u->ypos) & (h - 1)) == 0 && u->line < UVI_HEIGHT) {
                if (layers & UVI_LAYER_BG) {
                    draw_row(u, u->charline * h + u->ypos, u->reg + UVI_CHARS1 + u->charline * 16);
                } else {
                    plot_box(u, u->charline * h + u->ypos, h, PAL(u, 1) & 7);
                    memset(u->bg[u->charline * h + u->ypos], 0, sizeof u->bg[0] * (size_t)1);
                }
            }
        } else if (u->lines26 && u->charline < 26) {
            if (((u->line - u->ypos) & (h - 1)) == 0 && u->line < UVI_HEIGHT) {
                if (layers & UVI_LAYER_BG) {
                    draw_row(u, u->charline * h + u->ypos,
                             u->reg + UVI_CHARS2 + (u->charline - 13) * 16);
                } else {
                    plot_box(u, u->charline * h + u->ypos, h, PAL(u, 1) & 7);
                }
            }
            u->charline -= 13;
        } else {
            u->charline = 0xd;
            plot_box(u, u->line, 1, PAL(u, 1) & 7);
            if (u->line < UVI_HEIGHT)
                memset(u->bg[u->line], 0, sizeof u->bg[0]);
        }
    }
    if (u->line == 261 && (layers & UVI_LAYER_SPRITES))
        draw_sprites(u);
    else if (u->line == 261) {
        u->reg[UVI_COLL_BG] |= 0xf;
        u->reg[UVI_COLL_SPR] |= 0x3f;
    }
}

void uvi_line(uvi_t *u)
{
    line_impl(u, UVI_LAYER_BG | UVI_LAYER_SPRITES);
}

/* ---- the debugger's views ---- */

void uvi_snapshot_get(const uvi_t *u, uvi_snapshot *s)
{
    memset(s, 0, sizeof *s);
    memcpy(s->reg, u->reg, UVI_SIZE);
    for (int i = 0; i < 4; i++) {
        uvi_object_info *o = &s->obj[i];
        const uint8_t pal = u->reg[UVI_PAL + (i < 2 ? 3 : 2)];

        o->reg_y = u->reg[UVI_OBJ + i * 2];
        o->reg_x = u->reg[UVI_OBJ + i * 2 + 1];
        o->x = u->pos[i].x;
        o->y = u->pos[i].y;
        o->colour = (i & 1) ? pal & 7 : (pal >> 3) & 7;
        o->double_height = !(pal & ((i & 1) ? 0x40 : 0x80));
        o->visible = !(o->y <= -UVI_YPOS || o->y >= UVI_HEIGHT - UVI_YPOS - 8 ||
                       o->x <= -UVI_XPOS || o->x >= 128 + UVI_XPOS - 8);
        memcpy(o->shape, u->reg + UVI_UDC + i * 8, 8);
    }
    s->line = u->line;
    s->charline = u->charline;
    s->ypos = u->ypos;
    s->shift = u->shift;
    s->graphics = u->graphics != 0;
    s->doublescan = u->doublescan != 0;
    s->lines26 = u->lines26 != 0;
    s->multicolor = u->multicolor != 0;
    s->ad_select = u->ad_select != 0;
    s->screen_colour = u->reg[UVI_PAL + 1] & 7;
    s->char_colour = (u->reg[UVI_PAL + 1] >> 3) & 7;
    s->alt_screen_colour = u->reg[UVI_PAL] & 7;
    s->alt_char_colour = (u->reg[UVI_PAL] >> 3) & 7;
    s->sound_freq = u->reg[UVI_SOUND1] & 0x7f;
    s->sound_volume = u->reg[UVI_SOUND2] & 7;
    s->sound_on = (u->reg[UVI_SOUND2] >> 3) & 1;
    s->sound_noise = (u->reg[UVI_SOUND2] >> 4) & 1;
    s->coll_bg = u->reg[UVI_COLL_BG];
    s->coll_spr = u->reg[UVI_COLL_SPR];
    memcpy(s->chars, u->chars, sizeof s->chars);
    s->vblank = uvi_vblank(u);
}

void uvi_describe_register(const uvi_t *u, unsigned offset, char *name, size_t name_n,
                           char *text, size_t text_n)
{
    char nm[48] = "", tx[96] = "";
    const uint8_t v = u->reg[offset & 0x3ff];

    offset &= 0x3ff;
    if (offset < UVI_RAM1) {
        snprintf(nm, sizeof nm, "SCR1+%02X", offset);
    } else if (offset < UVI_OBJ) {
        snprintf(nm, sizeof nm, "RAM1+%02X", offset - UVI_RAM1);
    } else if (offset < UVI_OBJ + 8) {
        const int i = (int)(offset - UVI_OBJ) >> 1;

        snprintf(nm, sizeof nm, "OBJ%d_%c", i, (offset & 1) ? 'X' : 'Y');
        if (offset & 1)
            snprintf(tx, sizeof tx, "x=%d", v - 43);
        else
            snprintf(tx, sizeof tx, "y=%d", (v ^ 0xff) + 1);
    } else if (offset < UVI_VPOS) {
        snprintf(nm, sizeof nm, "RAM2+%X", offset - 0xf8);
    } else if (offset == UVI_VPOS) {
        snprintf(nm, sizeof nm, "VPOS");
        snprintf(tx, sizeof tx, "first character line %d", 255 - v + UVI_YPOS);
    } else if (offset == UVI_SOUND1) {
        snprintf(nm, sizeof nm, "SND1");
        snprintf(tx, sizeof tx, "pitch %d, %s colours", v & 0x7f, (v & 0x80) ? "multi" : "single");
    } else if (offset == UVI_SOUND2) {
        snprintf(nm, sizeof nm, "SND2");
        snprintf(tx, sizeof tx, "vol %d%s%s, scroll %d", v & 7, (v & 8) ? " tone" : "",
                 (v & 0x10) ? " noise" : "", v >> 5);
    } else if (offset == UVI_CHARLINE) {
        snprintf(nm, sizeof nm, "CHARLINE");
        snprintf(tx, sizeof tx, "now %X", u->charline);
    } else if (offset >= UVI_PAD && offset < UVI_PAD + 8) {
        snprintf(nm, sizeof nm, "PAD%d%c", (int)(offset - UVI_PAD) / 4 + 1, 'A' + (int)(offset & 3));
        snprintf(tx, sizeof tx, "reads %02X", uvi_read(u, offset));
    } else if (offset == UVI_KEYS) {
        snprintf(nm, sizeof nm, "PANEL");
        snprintf(tx, sizeof tx, "reads %02X", uvi_read(u, offset));
    } else if (offset >= UVI_UDC && offset < UVI_UDC + 0x40) {
        const int n = (int)(offset - UVI_UDC) >> 3;

        snprintf(nm, sizeof nm, "UDC%02X+%d", 0x38 + n, (int)offset & 7);
        if (n < 4)
            snprintf(tx, sizeof tx, "also object %d", n);
    } else if (offset == UVI_PAL) {
        snprintf(nm, sizeof nm, "PAL0");
        snprintf(tx, sizeof tx, "alt screen %s, alt chars %s, %s%s",
                 uvi_colour_name(v), uvi_colour_name(v >> 3),
                 (v & 0x40) ? "two screens" : "one screen", (v & 0x80) ? ", block graphics" : "");
    } else if (offset == UVI_PAL + 1) {
        snprintf(nm, sizeof nm, "PAL1");
        snprintf(tx, sizeof tx, "screen %s, chars %s, pot %s, chars %s", uvi_colour_name(v),
                 uvi_colour_name(v >> 3), (v & 0x40) ? "x" : "y", (v & 0x80) ? "8x8" : "8x16");
    } else if (offset == UVI_PAL + 2) {
        snprintf(nm, sizeof nm, "PAL2");
        snprintf(tx, sizeof tx, "obj3 %s %s, obj2 %s %s", uvi_colour_name(v),
                 (v & 0x40) ? "8x8" : "8x16", uvi_colour_name(v >> 3), (v & 0x80) ? "8x8" : "8x16");
    } else if (offset == UVI_PAL + 3) {
        snprintf(nm, sizeof nm, "PAL3");
        snprintf(tx, sizeof tx, "obj1 %s %s, obj0 %s %s", uvi_colour_name(v),
                 (v & 0x40) ? "8x8" : "8x16", uvi_colour_name(v >> 3), (v & 0x80) ? "8x8" : "8x16");
    } else if (offset == UVI_COLL_BG) {
        snprintf(nm, sizeof nm, "COLL_BG");
        snprintf(tx, sizeof tx, "objects hitting characters: %X (0 = hit)", v & 0xf);
    } else if (offset == UVI_COLL_SPR) {
        snprintf(nm, sizeof nm, "COLL_OBJ");
        snprintf(tx, sizeof tx, "object pairs hitting: %02X (0 = hit)", v & 0x3f);
    } else if (offset == UVI_POT1 || offset == UVI_POT2) {
        snprintf(nm, sizeof nm, "POT%d", offset == UVI_POT2 ? 1 : 2);
        snprintf(tx, sizeof tx, "reads %02X", uvi_read(u, offset));
    } else if (offset >= UVI_CHARS2 && offset < UVI_RAM3) {
        snprintf(nm, sizeof nm, "SCR2+%02X", offset - UVI_CHARS2);
    } else if (offset >= UVI_RAM3) {
        snprintf(nm, sizeof nm, "RAM3+%02X", offset - UVI_RAM3);
    } else {
        snprintf(nm, sizeof nm, "UVI+%03X", offset);
    }
    if (name && name_n)
        snprintf(name, name_n, "%s", nm);
    if (text && text_n)
        snprintf(text, text_n, "%s", tx);
}

void uvi_debug_render(const uvi_t *u, unsigned layers, uint32_t *dst)
{
    uvi_t *t = malloc(sizeof *t);

    if (!t) {
        memset(dst, 0, sizeof(uint32_t) * UVI_WIDTH * UVI_HEIGHT);
        return;
    }
    *t = *u;
    t->line = 0;
    for (int i = 0; i < t->lines; i++)
        line_impl(t, layers);
    memcpy(dst, t->bitmap, sizeof t->bitmap);
    free(t);
}

void uvi_frame_in_progress(const uvi_t *u, uint32_t *dst)
{
    memcpy(dst, u->bitmap, sizeof u->bitmap);
}

static int sheet_check(size_t need, size_t cap)
{
    return need <= cap ? 0 : -1;
}

int uvi_render_glyph(const uvi_t *u, int code, int scale, uint32_t fg, uint32_t bg,
                     uint32_t *dst, size_t cap, int *w, int *h)
{
    const int n = 8 * scale;

    if (scale < 1 || sheet_check((size_t)n * (size_t)n, cap))
        return -1;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++)
            dst[y * n + x] = (u->chars[code & 0x3f][y / scale] & (0x80 >> (x / scale))) ? fg : bg;
    *w = *h = n;
    return 0;
}

int uvi_render_font_sheet(const uvi_t *u, int scale, uint32_t fg, uint32_t bg,
                          uint32_t *dst, size_t cap, int *w, int *h)
{
    const int cell = 8 * scale + 2;
    const int cols = 16, rows = 4;
    const int W = cols * cell, H = rows * cell;

    if (scale < 1 || sheet_check((size_t)W * (size_t)H, cap))
        return -1;
    for (int i = 0; i < W * H; i++)
        dst[i] = 0x00000000;
    for (int code = 0; code < 0x40; code++) {
        const int ox = (code % cols) * cell + 1, oy = (code / cols) * cell + 1;

        for (int y = 0; y < 8 * scale; y++)
            for (int x = 0; x < 8 * scale; x++)
                dst[(oy + y) * W + ox + x] =
                    (u->chars[code][y / scale] & (0x80 >> (x / scale))) ? fg : bg;
    }
    *w = W;
    *h = H;
    return 0;
}

int uvi_render_object(const uvi_t *u, int index, int scale, uint32_t bg,
                      uint32_t *dst, size_t cap, int *w, int *h)
{
    uvi_snapshot s;
    const int W = 8 * scale;
    int H;

    uvi_snapshot_get(u, &s);
    index &= 3;
    H = (s.obj[index].double_height ? 16 : 8) * scale;
    if (scale < 1 || sheet_check((size_t)W * (size_t)H, cap))
        return -1;
    for (int y = 0; y < H; y++) {
        const int row = s.obj[index].double_height ? y / (2 * scale) : y / scale;

        for (int x = 0; x < W; x++)
            dst[y * W + x] = (s.obj[index].shape[row] & (0x80 >> (x / scale)))
                                 ? uvi_colours[s.obj[index].colour] : bg;
    }
    *w = W;
    *h = H;
    return 0;
}

int uvi_render_palette_strip(int cell, uint32_t *dst, size_t cap, int *w, int *h)
{
    const int W = 8 * cell;

    if (cell < 1 || sheet_check((size_t)W * (size_t)cell, cap))
        return -1;
    for (int y = 0; y < cell; y++)
        for (int x = 0; x < W; x++)
            dst[y * W + x] = uvi_colours[x / cell];
    *w = W;
    *h = cell;
    return 0;
}
