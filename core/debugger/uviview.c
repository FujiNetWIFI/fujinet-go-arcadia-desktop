/*
 * uviview.c -- the debugger's glue for the chip that draws, speaks and reads
 * the keypads: the Signetics 2637 UVI. Thin on purpose: the chip decodes its
 * own registers (uvi_snapshot_get, uvi_describe_register, the renderers);
 * this file takes the host lock, flattens the chip's state into the public
 * arcdebug_uvi, and does the things the chip does not -- the character
 * screen's per-cell colours and block-graphics switches, the overlays on a
 * picture, scaling an image up, deciding which object the pointer is over,
 * and printing the chip as prompt text.
 *
 * This file is also the marker that the debugger is ported: core/CMakeLists.txt
 * compiles the debugger once it exists.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "debugger_internal.h"
#include "host.h"

#define FRAME_W UVI_WIDTH
#define FRAME_H UVI_HEIGHT

#define VIEW_FG 0x00e0e0e0u
#define VIEW_BG 0x00101010u

/* the object pairs $19FD reports: bit n of the register, low = a hit */
static const uint8_t pair_a[6] = { 0, 0, 0, 1, 1, 2 };
static const uint8_t pair_b[6] = { 1, 2, 3, 2, 3, 3 };

/* ---- the snapshot, the registers ------------------------------------------- */

static void get_locked(arc_machine_t *m, arcdebug_uvi *out)
{
    uvi_snapshot s;

    memset(out, 0, sizeof *out);
    if (!m)
        return;
    uvi_snapshot_get(&m->uvi, &s);
    memcpy(out->reg, s.reg, sizeof out->reg);
    for (int i = 0; i < 4; i++)
    {
        arcdebug_uvi_object *o = &out->obj[i];
        const uvi_object_info *u = &s.obj[i];

        o->index = i;
        o->x = u->x;
        o->y = u->y;
        o->px = u->x + UVI_XPOS;
        o->py = u->y + UVI_YPOS;
        o->reg_x = u->reg_x;
        o->reg_y = u->reg_y;
        o->colour = u->colour;
        o->double_height = u->double_height;
        o->width = 8;
        o->height = u->double_height ? 16 : 8;
        o->udc = 0x38 + i;
        o->visible = u->visible;
        memcpy(o->shape, u->shape, 8);
        o->hits_chars = !(s.coll_bg & (1u << i));
    }
    for (int p = 0; p < 6; p++)
        if (!(s.coll_spr & (1u << p)))
        {
            out->obj[pair_a[p]].hits_objects |= 1 << pair_b[p];
            out->obj[pair_b[p]].hits_objects |= 1 << pair_a[p];
        }
    out->line = s.line;
    out->charline = s.charline;
    out->vblank = s.vblank;
    out->first_line = s.ypos;
    out->shift = s.shift;
    out->rows = s.lines26 ? 26 : 13;
    out->cell_height = s.doublescan ? 16 : 8;
    out->two_screens = s.lines26;
    out->block_graphics = s.graphics;
    out->multicolor = s.multicolor;
    out->pot_y = !(s.reg[UVI_PAL + 1] & 0x40);
    out->screen_colour = s.screen_colour;
    out->char_colour = s.char_colour;
    out->alt_screen_colour = s.alt_screen_colour;
    out->alt_char_colour = s.alt_char_colour;
    out->coll_chars = s.coll_bg;
    out->coll_objects = s.coll_spr;
    out->sound_pitch = s.reg[UVI_SOUND1] & 0x7f;
    out->sound_volume = s.sound_volume;
    out->sound_tone = s.sound_on;
    out->sound_noise = s.sound_noise;
    for (int i = 0; i < 8; i++)
        out->pad[i] = uvi_read(&m->uvi, UVI_PAD + (unsigned)i);
    out->panel = uvi_read(&m->uvi, UVI_KEYS);
    out->pot[0] = uvi_read(&m->uvi, UVI_POT1);
    out->pot[1] = uvi_read(&m->uvi, UVI_POT2);
    memcpy(out->chars, s.chars, sizeof out->chars);
    out->lines = m->uvi.lines;
}

void arcdebug_uvi_get(arcdebug *d, arcdebug_uvi *out)
{
    (void)d;
    arc_host_lock();
    get_locked(arcdebug_machine(), out);
    arc_host_unlock();
}

/* Flatten the character screen with the chip's own rules (uvi.c draw_row,
 * draw_char). */
static int cells_locked(arc_machine_t *m, arcdebug_uvi_cell *cells)
{
    const uvi_t *u;
    int rows;

    if (!m)
        return 0;
    u = &m->uvi;
    rows = u->lines26 ? 26 : 13;
    for (int r = 0; r < rows; r++)
    {
        const uint8_t *src = u->reg + (r < 13 ? UVI_CHARS1 + r * 16 : UVI_CHARS2 + (r - 13) * 16);
        int graphics = u->graphics != 0;

        for (int c = 0; c < 16; c++)
        {
            const uint8_t ch = src[c];
            arcdebug_uvi_cell *cell = &cells[r * 16 + c];
            const uint8_t pal0 = u->reg[UVI_PAL], pal1 = u->reg[UVI_PAL + 1];
            int cc, sc;

            if ((ch & 0x3f) == 0)
            {
                if (ch == 0xc0)
                    graphics = 1;
                else if (ch == 0x40)
                    graphics = 0;
            }
            if (u->multicolor)
            {
                cc = ((ch & 0x40 ? pal1 : pal0) >> 3) & 7;
                sc = (ch & 0x80 ? pal1 : pal0) & 7;
            }
            else
            {
                cc = ((pal1 >> 3) & 1) | ((ch >> 5) & 6);
                sc = pal1 & 7;
            }
            cell->code = ch;
            cell->glyph = ch & 0x3f;
            cell->graphics = (uint8_t)graphics;
            cell->fg = (uint8_t)cc;
            cell->bg = (uint8_t)sc;
        }
    }
    return rows;
}

int arcdebug_uvi_cells(arcdebug *d, arcdebug_uvi_cell *cells)
{
    int rows;
    (void)d;
    arc_host_lock();
    rows = cells_locked(arcdebug_machine(), cells);
    arc_host_unlock();
    return rows;
}

void arcdebug_uvi_describe(arcdebug *d, unsigned offset, char *name, size_t name_n,
                           char *text, size_t text_n)
{
    arc_machine_t *m;

    (void)d;
    if (name && name_n)
        name[0] = '\0';
    if (text && text_n)
        text[0] = '\0';
    arc_host_lock();
    m = arcdebug_machine();
    if (m)
        uvi_describe_register(&m->uvi, offset % UVI_SIZE, name, name_n, text, text_n);
    arc_host_unlock();
}

void arcdebug_uvi_write(arcdebug *d, unsigned offset, uint8_t value)
{
    arcdebug_write(d, ARC_SPACE_UVI, offset, value);
}

const char *arcdebug_uvi_colour_name(int colour)
{
    return uvi_colour_name(colour & 7);
}

/* ---- the views -------------------------------------------------------------- */

/* Nearest-neighbour: every source pixel becomes scale x scale. */
static void upscale(const uint32_t *src, int w, int h, int scale, uint32_t *dst)
{
    const size_t dw = (size_t)w * (size_t)scale;

    for (int y = 0; y < h; y++)
    {
        uint32_t *row = dst + (size_t)y * (size_t)scale * dw;

        for (int x = 0; x < w; x++)
            for (int k = 0; k < scale; k++)
                row[(size_t)x * (size_t)scale + (size_t)k] = src[(size_t)y * (size_t)w + (size_t)x];
        for (int k = 1; k < scale; k++)
            memcpy(row + (size_t)k * dw, row, dw * sizeof *row);
    }
}

static void put_px(uint32_t *img, int x, int y, uint32_t c)
{
    if (x >= 0 && x < FRAME_W && y >= 0 && y < FRAME_H)
        img[y * FRAME_W + x] = c;
}

static void outline(uint32_t *img, int x, int y, int w, int h, uint32_t c)
{
    for (int i = 0; i < w; i++)
    {
        put_px(img, x + i, y, c);
        put_px(img, x + i, y + h - 1, c);
    }
    for (int j = 0; j < h; j++)
    {
        put_px(img, x, y + j, c);
        put_px(img, x + w - 1, y + j, c);
    }
}

static void overlays(arc_machine_t *m, uint32_t *img, unsigned flags)
{
    static const uint32_t box[4] = { 0x00ff4040, 0x0040c040, 0x004080ff, 0x00ffa000 };
    arcdebug_uvi u;

    if (!flags)
        return;
    get_locked(m, &u);
    if (flags & ARCDEBUG_UVI_F_GRID)
    {
        const int x0 = UVI_XPOS + u.shift;

        for (int r = 0; r < u.rows; r++)
            for (int c = 0; c < 16; c++)
            {
                const int x = x0 + c * 8, y = u.first_line + r * u.cell_height;
                for (int i = 0; i < 8; i += 2)
                {
                    put_px(img, x + i, y, 0x00808080);
                    put_px(img, x, y + i, 0x00808080);
                }
            }
    }
    if (flags & (ARCDEBUG_UVI_F_BOXES | ARCDEBUG_UVI_F_COLLIDE))
        for (int i = 0; i < 4; i++)
        {
            const arcdebug_uvi_object *o = &u.obj[i];
            const int hit = o->hits_chars || o->hits_objects;

            if (!o->visible)
                continue;
            if ((flags & ARCDEBUG_UVI_F_COLLIDE) && hit)
                outline(img, o->px - 1, o->py - 1, o->width + 2, o->height + 2, 0x00ff00ff);
            else if (flags & ARCDEBUG_UVI_F_BOXES)
                outline(img, o->px - 1, o->py - 1, o->width + 2, o->height + 2, box[i]);
        }
    if (flags & ARCDEBUG_UVI_F_BEAM)
    {
        int vp, hp;
        arcdebug_beam(m, &vp, &hp);
        for (int x = 0; x < FRAME_W; x += 2)
            put_px(img, x, vp, 0x00ff8000);
        for (int y = 0; y < FRAME_H; y += 2)
            put_px(img, hp, y, 0x00ff8000);
    }
}

int arcdebug_uvi_view(arcdebug *d, int view, unsigned layers, unsigned flags,
                      int scale, uint32_t *dst, size_t cap, int *w, int *h)
{
    /* one frame's worth of scratch; the host lock serialises its users */
    static uint32_t tmp[FRAME_W * FRAME_H];
    arc_machine_t *m;
    int ok = 0, rw = 0, rh = 0;

    (void)d;
    if (scale < 1)
        scale = 1;
    arc_host_lock();
    m = arcdebug_machine();
    switch (view)
    {
    case ARCDEBUG_UVI_FRAME:
    case ARCDEBUG_UVI_RENDER:
    case ARCDEBUG_UVI_PROGRESS:
        rw = FRAME_W * scale;
        rh = FRAME_H * scale;
        if (!m)
            break;
        ok = 1;
        if (!dst)
            break;
        if (cap < (size_t)rw * (size_t)rh)
        {
            ok = 0;
            break;
        }
        if (view == ARCDEBUG_UVI_FRAME)
            memcpy(tmp, m->fb, sizeof tmp);
        else if (view == ARCDEBUG_UVI_RENDER)
            uvi_debug_render(&m->uvi, layers & ARCDEBUG_UVI_L_ALL, tmp);
        else
        {
            uvi_frame_in_progress(&m->uvi, tmp);
            /* what the beam has not reached yet still shows the last frame */
            for (int y = m->uvi.line + 1; y < FRAME_H; y++)
                for (int x = 0; x < FRAME_W; x++)
                    tmp[y * FRAME_W + x] = (tmp[y * FRAME_W + x] >> 1) & 0x007f7f7fu;
        }
        overlays(m, tmp, flags);
        if (scale == 1)
            memcpy(dst, tmp, sizeof tmp);
        else
            upscale(tmp, FRAME_W, FRAME_H, scale, dst);
        break;
    case ARCDEBUG_UVI_FONT:
        if (m)
        {
            rw = 16 * (8 * scale + 2);
            rh = 4 * (8 * scale + 2);
            ok = 1;
            if (dst)
                ok = uvi_render_font_sheet(&m->uvi, scale, VIEW_FG, VIEW_BG, dst, cap, &rw, &rh) == 0;
        }
        break;
    case ARCDEBUG_UVI_OBJECTS:
    {
        const int gap = 2 * scale, cw = 8 * scale, ch = 16 * scale;

        rw = 4 * cw + 3 * gap;
        rh = ch;
        if (!m)
            break;
        ok = 1;
        if (!dst)
            break;
        if (cap < (size_t)rw * (size_t)rh)
        {
            ok = 0;
            break;
        }
        for (int i = 0; i < rw * rh; i++)
            dst[i] = VIEW_BG;
        for (int i = 0; i < 4; i++)
        {
            uint32_t one[8 * 16 * 16 * 16];
            int ow = 0, oh = 0;

            if (scale > 16)
                break;
            if (uvi_render_object(&m->uvi, i, scale, VIEW_BG, one, sizeof one / sizeof one[0],
                                  &ow, &oh) != 0)
                continue;
            for (int y = 0; y < oh; y++)
                memcpy(dst + (size_t)y * (size_t)rw + (size_t)(i * (cw + gap)),
                       one + (size_t)y * (size_t)ow, (size_t)ow * sizeof *dst);
        }
        break;
    }
    case ARCDEBUG_UVI_PALETTE:
        if (m)
        {
            const int cell = 16 * scale;
            if (!dst)
            {
                rw = 8 * cell;
                rh = cell;
                ok = 1;
            }
            else
                ok = uvi_render_palette_strip(cell, dst, cap, &rw, &rh) == 0;
        }
        break;
    default:
        break;
    }
    arc_host_unlock();
    if (ok)
    {
        if (w)
            *w = rw;
        if (h)
            *h = rh;
    }
    return ok;
}

int arcdebug_uvi_thing(arcdebug *d, int kind, int index, int scale, uint32_t bg,
                       uint32_t *dst, size_t cap, int *w, int *h)
{
    arc_machine_t *m;
    int ok = 0, rw = 0, rh = 0;

    (void)d;
    if (scale < 1)
        scale = 1;
    arc_host_lock();
    m = arcdebug_machine();
    if (m)
    {
        if (kind == ARCDEBUG_UVI_OBJECT)
        {
            uvi_snapshot s;
            uvi_snapshot_get(&m->uvi, &s);
            rw = 8 * scale;
            rh = (s.obj[index & 3].double_height ? 16 : 8) * scale;
            ok = 1;
            if (dst)
                ok = uvi_render_object(&m->uvi, index & 3, scale, bg, dst, cap, &rw, &rh) == 0;
        }
        else if (kind == ARCDEBUG_UVI_GLYPH)
        {
            rw = rh = 8 * scale;
            ok = 1;
            if (dst)
                ok = uvi_render_glyph(&m->uvi, index & 0x3f, scale,
                                      uvi_colours[(m->uvi.reg[UVI_PAL + 1] >> 3) & 7], bg, dst,
                                      cap, &rw, &rh) == 0;
        }
    }
    arc_host_unlock();
    if (ok)
    {
        if (w)
            *w = rw;
        if (h)
            *h = rh;
    }
    return ok;
}

int arcdebug_uvi_hit_test(arcdebug *d, int x, int y, arcdebug_hit *out, int max)
{
    arc_machine_t *m;
    arcdebug_uvi u;
    int n = 0;

    (void)d;
    arc_host_lock();
    m = arcdebug_machine();
    if (m)
    {
        get_locked(m, &u);
        for (int i = 3; i >= 0 && n < max; i--)
        {
            const arcdebug_uvi_object *o = &u.obj[i];
            const int dx = x - o->px, dy = y - o->py;

            if (!o->visible || dx < 0 || dx >= o->width || dy < 0 || dy >= o->height)
                continue;
            if (o->shape[o->double_height ? dy / 2 : dy] & (0x80 >> dx))
            {
                out[n].kind = ARCDEBUG_UVI_HIT_OBJECT;
                out[n++].index = i;
            }
        }
        {
            const int cx = x - (UVI_XPOS + u.shift), cy = y - u.first_line;

            if (n < max && cx >= 0 && cx < 128 && cy >= 0 && cy < u.rows * u.cell_height)
            {
                out[n].kind = ARCDEBUG_UVI_HIT_CELL;
                out[n++].index = (cy / u.cell_height) * 16 + cx / 8;
            }
        }
    }
    arc_host_unlock();
    return n;
}

/* ---- text for the prompt -------------------------------------------------------- */

typedef struct {
    char *dst;
    int cap, len;
} textbuf;

static void put(textbuf *t, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (t->len >= t->cap - 1)
        return;
    va_start(ap, fmt);
    n = vsnprintf(t->dst + t->len, (size_t)(t->cap - t->len), fmt, ap);
    va_end(ap);
    if (n > 0)
        t->len += n < t->cap - t->len ? n : t->cap - t->len - 1;
}

int arcdebug_text_uvi(char *dst, int dstsz)
{
    textbuf t = { dst, dstsz, 0 };
    arc_machine_t *m;

    if (dstsz <= 0)
        return 0;
    dst[0] = '\0';
    arc_host_lock();
    m = arcdebug_machine();
    if (!m)
        put(&t, "the machine is not running\n");
    else
    {
        arcdebug_uvi u;
        char name[48], text[160];
        int vp, hp;

        get_locked(m, &u);
        arcdebug_beam(m, &vp, &hp);
        put(&t, "2637 UVI (%s): beam line %d pixel %d, %d lines/frame, frame %llu%s\n",
            m->is_pal ? "PAL" : "NTSC", vp, hp, u.lines, (unsigned long long)m->frames_done,
            u.vblank ? ", vblank" : "");
        put(&t, "screen: %d rows of 16 from line %d, cells 8x%d, shift %d%s%s%s\n", u.rows,
            u.first_line, u.cell_height, u.shift, u.two_screens ? ", two screens" : "",
            u.block_graphics ? ", block graphics" : "", u.multicolor ? ", multicolor" : "");
        put(&t, "colours: screen %s, characters %s; alternate %s / %s\n",
            uvi_colour_name(u.screen_colour), uvi_colour_name(u.char_colour),
            uvi_colour_name(u.alt_screen_colour), uvi_colour_name(u.alt_char_colour));
        put(&t, "collisions: characters %X, objects %02X (0 = hit)\n", u.coll_chars & 0xf,
            u.coll_objects & 0x3f);
        put(&t, "sound: pitch %d, volume %d%s%s\n", u.sound_pitch, u.sound_volume,
            u.sound_tone ? ", tone" : "", u.sound_noise ? ", noise" : "");
        static const unsigned ranges[][2] = { { UVI_OBJ, UVI_OBJ + 8 }, { UVI_VPOS, UVI_CHARLINE + 1 },
                                              { UVI_PAD, UVI_KEYS + 1 }, { UVI_PAL, UVI_POT2 + 1 } };
        for (unsigned r = 0; r < sizeof ranges / sizeof ranges[0]; r++)
            for (unsigned a = ranges[r][0]; a < ranges[r][1]; a++)
            {
                uvi_describe_register(&m->uvi, a, name, sizeof name, text, sizeof text);
                put(&t, "$%04X %-9s %02X  %s\n", 0x1800 + a, name, u.reg[a], text);
            }
    }
    arc_host_unlock();
    return t.len;
}

int arcdebug_text_objects(char *dst, int dstsz)
{
    textbuf t = { dst, dstsz, 0 };
    arc_machine_t *m;

    if (dstsz <= 0)
        return 0;
    dst[0] = '\0';
    arc_host_lock();
    m = arcdebug_machine();
    if (!m)
        put(&t, "the machine is not running\n");
    else
    {
        arcdebug_uvi u;

        get_locked(m, &u);
        for (int i = 0; i < 4; i++)
        {
            const arcdebug_uvi_object *o = &u.obj[i];

            put(&t, "object %d  x %4d y %4d (regs %02X %02X)  %s  8x%d  UDC $%02X  %s%s%s\n", i, o->x,
                o->y, o->reg_x, o->reg_y, uvi_colour_name(o->colour), o->height, o->udc,
                o->visible ? "visible" : "hidden", o->hits_chars ? ", hit characters" : "",
                o->hits_objects ? ", hit objects" : "");
            put(&t, "          shape");
            for (int k = 0; k < 8; k++)
                put(&t, " %02X", o->shape[k]);
            put(&t, "\n");
        }
    }
    arc_host_unlock();
    return t.len;
}

int arcdebug_text_screen(char *dst, int dstsz)
{
    textbuf t = { dst, dstsz, 0 };
    arc_machine_t *m;

    if (dstsz <= 0)
        return 0;
    dst[0] = '\0';
    arc_host_lock();
    m = arcdebug_machine();
    if (!m)
        put(&t, "the machine is not running\n");
    else
    {
        arcdebug_uvi_cell cells[ARCDEBUG_UVI_COLS * ARCDEBUG_UVI_MAX_ROWS];
        const int rows = cells_locked(m, cells);

        for (int r = 0; r < rows; r++)
        {
            put(&t, "%2d ", r);
            for (int c = 0; c < 16; c++)
                put(&t, " %02X%s", cells[r * 16 + c].code, cells[r * 16 + c].graphics ? "g" : " ");
            put(&t, "\n");
        }
        put(&t, "(g = drawn as block graphics)\n");
    }
    arc_host_unlock();
    return t.len;
}
