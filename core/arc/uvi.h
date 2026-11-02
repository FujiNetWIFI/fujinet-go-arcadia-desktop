/* uvi.h -- the Signetics 2637 UVI (Universal Video Interface), the Emerson
 * Arcadia 2001's video, keypad decode and pot inputs.
 *
 * Transposed from MAME's src/mame/arcadia/arcadia_v.cpp and arcadia.h
 * (GPL-2.0-or-later, copyright-holders Peter Trauner, consolidation by Manfred
 * Schneider; see COMPLIANCE.md). Field names keep MAME's own, minus the m_
 * prefix.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---------------------------------------------------------------------------
 * HOW IT IS DRIVEN
 * ---------------------------------------------------------------------------
 *
 * The UVI is a register file at the CPU's $1800-$1AFF (offsets $000-$2FF
 * here) plus a line renderer. MAME draws one display line per tick of a
 * 262 * 60 Hz timer (the s2650's set_periodic_int, video_line) and nothing
 * else: there is no clock inside the chip. uvi_line() is that tick; the
 * machine calls it at the CPU time MAME's scheduler would (machine.c), 262
 * times per NTSC frame (312 per PAL one). Each call draws the character row
 * that starts on that line from the registers AS THEY ARE THEN, so a
 * mid-frame register write lands on the right line exactly as in MAME.
 * The bitmap is persistent: a line is overwritten only by its own tick.
 * Sprites are drawn over the finished picture on line 261 (sprite 0 of
 * line 0 is erased again by the next tick, a MAME property kept).
 *
 * The picture is ARC_FB_WIDTH x ARC_FB_HEIGHT (192 x 262): 128 pixels of
 * screen with 32 of border each side (XPOS), XRGB 0x00RRGGBB.
 *
 * INPUTS are not read from the machine: the host stores them in `in` at each
 * frame boundary (keypad columns, panel, the four pot values) and uvi_read()
 * returns them as MAME's video_r does.
 *
 * The debug functions are const and touch no chip state.
 */

#ifndef ARC_UVI_H
#define ARC_UVI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UVI_XPOS      32            /* the border to the left of the 128-pixel screen */
#define UVI_YPOS      0
#define UVI_WIDTH     (128 + 2 * UVI_XPOS)
#define UVI_HEIGHT    262           /* MAME's screen is 262 tall on every clone */
#define UVI_BG_BYTES  (16 + 2 * UVI_XPOS / 8)

/* register file offsets (from $1800) */
#define UVI_CHARS1    0x000         /* upper screen: 13 rows of 16 */
#define UVI_RAM1      0x0d0
#define UVI_OBJ       0x0f0         /* y0 x0 y1 x1 y2 x2 y3 x3 */
#define UVI_VPOS      0x0fc
#define UVI_SOUND1    0x0fd
#define UVI_SOUND2    0x0fe
#define UVI_CHARLINE  0x0ff
#define UVI_PAD       0x100         /* $1900-$1907: pad1 a-d, pad2 a-d */
#define UVI_KEYS      0x108         /* the panel */
#define UVI_UDC       0x180         /* object images / UDCs $38-$3F: 8 x 8 */
#define UVI_PAL       0x1f8         /* $19F8-$19FB */
#define UVI_COLL_BG   0x1fc
#define UVI_COLL_SPR  0x1fd
#define UVI_POT1      0x1fe
#define UVI_POT2      0x1ff
#define UVI_CHARS2    0x200         /* lower screen: 13 rows of 16 */
#define UVI_RAM3      0x2d0
#define UVI_SIZE      0x300         /* offsets the CPU reaches ($1800-$1AFF) */

typedef struct {
    /* the keypad columns as the CPU reads them ($1900-$1907, bit i = key down;
     * the machine ORs 0xF0 into the pad bytes, as MAME's IPT_UNUSED does) and
     * the panel ($1908 b0 START, b1 OPTION, b2 SELECT) */
    uint8_t pad[8];
    uint8_t panel;
    /* the pots: [stick][axis] 0 = x (left-right), 1 = y (up-down); stick 0
     * is the CPU's $19FF, stick 1 its $19FE. The axis the CPU sees is the one
     * the mux (ad_select) picks. */
    uint8_t pot[2][2];
} uvi_inputs_t;

typedef struct {
    /* MAME's arcadia_state video members */
    int line, charline, shift, ad_delay, ad_select, ypos;
    int graphics, doublescan, lines26, multicolor;
    struct { int x, y; } pos[4];
    uint8_t bg[UVI_HEIGHT][UVI_BG_BYTES];
    uint8_t rectangle[0x40][8];
    uint8_t chars[0x40][8];
    uint8_t reg[0x400];                 /* m_reg.data */

    int lines;                          /* lines per frame: 262 / 312 */
    uint64_t line_count;                /* uvi_line() calls since power-on */
    uvi_inputs_t in;

    uint32_t bitmap[UVI_WIDTH * UVI_HEIGHT];    /* XRGB, MAME's m_bitmap */
} uvi_t;

/* Power on (MAME's video_start). `lines` is the frame height of the machine's
 * timer: 262 (NTSC) or 312 (PAL; MAME has no PAL timing: its clones run NTSC,
 * so the PAL row is not verified). */
void uvi_init(uvi_t *u, int lines);

/* video_r / video_w; offset 0-$2FF (the CPU address - $1800). */
uint8_t uvi_read(const uvi_t *u, unsigned offset);
void uvi_write(uvi_t *u, unsigned offset, uint8_t data);

/* One tick of video_line. Sprites are drawn on the last line of a frame. */
void uvi_line(uvi_t *u);

/* SENSE: vsync_r. */
static inline int uvi_vblank(const uvi_t *u) { return u->line >= 216; }

/* The 8 UVI colours (0 white ... 7 black) as XRGB. */
extern const uint32_t uvi_colours[8];
const char *uvi_colour_name(int index);

/* ---- the debugger's views ------------------------------------------------ */

typedef struct {
    int x, y;               /* MAME's (the register's x - 43, y = (reg ^ $FF) + 1) */
    uint8_t reg_y, reg_x;   /* as the CPU wrote them */
    int colour;             /* 0-7 */
    int double_height;      /* the size bit is low: 8x16 */
    int visible;            /* geometric: inside what draw_sprites draws */
    uint8_t shape[8];       /* the image, UDC $38 + n */
} uvi_object_info;

typedef struct {
    uint8_t reg[UVI_SIZE];
    uvi_object_info obj[4];
    int line;               /* the beam: MAME's m_line */
    int charline;
    int ypos;               /* first line of the character area */
    int shift;              /* fine scroll, 0-7 pixels */
    int graphics;           /* block graphics on */
    int doublescan;         /* 8x16 characters (the size bit low) */
    int lines26;            /* both screens shown (refresh mode) */
    int multicolor;
    int ad_select;
    int screen_colour, char_colour;          /* $19F9 */
    int alt_screen_colour, alt_char_colour;  /* $19F8 */
    int sound_freq, sound_volume, sound_on, sound_noise;
    uint8_t coll_bg, coll_spr;
    uint8_t chars[0x40][8]; /* the glyph table incl. the UDCs $38-$3F */
    int vblank;
} uvi_snapshot;

void uvi_snapshot_get(const uvi_t *u, uvi_snapshot *out);

/* Name and decoded text of the register at `offset` (the CPU-visible $18xx
 * names), as the debugger's register table lists them. */
void uvi_describe_register(const uvi_t *u, unsigned offset, char *name, size_t name_n,
                           char *text, size_t text_n);

/* Layers for uvi_debug_render. */
#define UVI_LAYER_BG       0x01     /* the character screen */
#define UVI_LAYER_SPRITES  0x02

/* Render the picture from the registers as they stand NOW (every row, then
 * the objects), into dst (UVI_WIDTH x UVI_HEIGHT XRGB). Touches no state: it
 * is what the frame would be had every line been drawn this instant. */
void uvi_debug_render(const uvi_t *u, unsigned layers, uint32_t *dst);

/* The frame as the beam has drawn it so far (MAME's bitmap: lines up to
 * `line` are this frame's, the rest still the last one's). */
void uvi_frame_in_progress(const uvi_t *u, uint32_t *dst);

/* Sheets of the glyph table, for the debugger's character view. `scale` is a
 * whole number of screen pixels per bit. Each returns 0, or -1 when the
 * buffer (cap in uint32 words) is too small; *w and *h get the size. */
int uvi_render_glyph(const uvi_t *u, int code, int scale, uint32_t fg, uint32_t bg,
                     uint32_t *dst, size_t cap, int *w, int *h);
int uvi_render_font_sheet(const uvi_t *u, int scale, uint32_t fg, uint32_t bg,
                          uint32_t *dst, size_t cap, int *w, int *h);
int uvi_render_object(const uvi_t *u, int index, int scale, uint32_t bg,
                      uint32_t *dst, size_t cap, int *w, int *h);
int uvi_render_palette_strip(int cell, uint32_t *dst, size_t cap, int *w, int *h);

#ifdef __cplusplus
}
#endif

#endif /* ARC_UVI_H */
