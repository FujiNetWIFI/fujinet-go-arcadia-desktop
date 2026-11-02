/*
 * arcdebug -- the debugger contract: a Signetics 2650 / 2637 UVI debugger engine
 * over the core (core/debugger/) behind a C API the four native debugger
 * windows share. Same shape as the Odyssey 2, Studio II and SMS siblings'
 * debug headers, so the windows carry over; this is that engine with the 8048
 * and the 8244 traded for the 2650 and the UVI, and the FujiNet cartridge's
 * mailbox at $2B00-$2FFF.
 *
 * Frontends consume ONLY this header (and arcsession.h, which it includes):
 * everything the windows show -- CPU, memory, disassembly, the video chip's
 * objects and character screen and registers, the cartridge, the FujiNet
 * packet trace -- comes through the calls below.
 *
 * The engine only engages while a window holds it attached:
 * arcdebug_attach() when the window opens -- which also stops the machine, as
 * on every sibling -- and arcdebug_detach() when it closes, which lets the
 * machine run on at full speed with no hook installed. The FujiNet trace is
 * the exception: it records from session start, so it shows what happened
 * before anyone opened a window.
 *
 * Calls are safe from the UI thread at any time. Inspection is meant for the
 * stopped state; while running it answers with whatever is current.
 *
 * ---- the machine, in one paragraph ----
 * The Emerson Arcadia 2001's CPU is a Signetics 2650 with a 15-bit address
 * space ($0000-$7FFF; the cartridge at $0000-$0FFF and, by mapper, $2000-$7FFF,
 * the UVI's registers at $1800-$1AFF, which include the screen RAM, the
 * object registers, the user-defined characters and the keypads, and nothing
 * else: no RAM, no BIOS). Addresses throughout this header are 15-bit PROG
 * addresses unless a function says otherwise. The 2650's page register (A14-A13)
 * is part of PC; an instruction's address arithmetic wraps inside its 8K page.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARCDEBUG_H
#define ARCDEBUG_H

#include <stddef.h>
#include <stdint.h>

#include "arcsession.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lazily created; lives as long as the session. */
arcdebug *arcdebug_get(arcsession *s);

/* ---- attach / stop / go ---------------------------------------------------- */
/* Engage the engine and stop at the next instruction. Idempotent. */
void arcdebug_attach(arcdebug *d);
/* Disengage and let the machine run. Breakpoints are kept for the next
 * attach. */
void arcdebug_detach(arcdebug *d);
int  arcdebug_is_attached(arcdebug *d);

int  arcdebug_is_stopped(arcdebug *d);
void arcdebug_stop(arcdebug *d);         /* break at the next instruction */
void arcdebug_resume(arcdebug *d);       /* "run" */
/* Why the machine last stopped ("stopped", "breakpoint: exec $0412",
 * "write prog $1801 = $12", "step", "frame", ...) and the PC, -1 if none or
 * running. Writes at most dstsz-1 bytes; returns the length. */
int  arcdebug_stop_reason(arcdebug *d, char *dst, int dstsz, int *address);
/* Bumped whenever a command ran or the machine stopped/resumed or something
 * was edited, so a window knows when to refresh. */
unsigned arcdebug_generation(arcdebug *d);

/* ---- the prompt ------------------------------------------------------------ */
/* Run one command; "help" lists them: step [n], over, out, frame [n],
 * scanline [n], run, stop, runto <addr>, break <addr>[-<end>] [cond],
 * bpr/bpw <addr>[-<end>] [cond] (reads/writes of PROG space, which includes
 * the UVI at $1800-$1AFF), delete <id|addr>, enable/disable <id>, clear,
 * breaks, print <expr>, regs, set <reg> <expr>, mem <space> <addr> [len]
 * (prog, uvi, cart), poke <space> <addr> <val...>, uvi, objects, screen, io,
 * keys, cart, mailbox, label <addr> <name>, labels [file], disasm [addr] [n],
 * trace [on|off|clear|n], save <kind> <file>.
 * Expressions: numbers ($0400, 0x400, 400h, 1024, %1010), registers (r0 r1-r3
 * [the set in use] r1'-r3' [set 1] pc iar psu psl sp cc c com wc rs idc ovf f
 * ii s), [addr] a PROG byte, p[addr] u[off] c[off] the three spaces (PROG,
 * UVI, CART), labels, vpos hpos frame cycles, in a breakpoint condition
 * `value` and `addr` for the access; C operators with C precedence. Output
 * text into dst; returns its length. */
int  arcdebug_command(arcdebug *d, const char *command, char *dst, int dstsz);
/* Completions for a prefix (commands, registers and labels), one per line.
 * Returns the count. */
int  arcdebug_completions(arcdebug *d, const char *prefix, char *dst, int dstsz);

/* ---- stepping shortcuts (the toolbar) -------------------------------------- */
/* All of these act only while stopped. An interrupt entry is taken together
 * with the first instruction of its handler: one step from an instruction an
 * interrupt precedes lands inside the handler's second instruction. */
void arcdebug_step(arcdebug *d);          /* F7: one instruction */
/* F8: over a BSTR/BSTA/BSFR/BSFA/BSNR/BSNA/BSXA/ZBSR (run until the
 * instruction after it is reached again with the return stack where it was);
 * any other instruction is a plain step. */
void arcdebug_step_over(arcdebug *d);
/* Shift+F8: to the routine's RETC/RETE that actually returns (a conditional
 * return not taken does not end it). */
void arcdebug_step_out(arcdebug *d);
void arcdebug_scanline(arcdebug *d, int n);   /* until the beam has moved n lines */
void arcdebug_frame(arcdebug *d, int n);      /* until n more frames are complete */
/* Run until PC == addr (15-bit). */
void arcdebug_run_to(arcdebug *d, uint16_t addr);

/* ---- CPU --------------------------------------------------------------------- */
typedef struct {
    int pc;                 /* 15 bits: page + iar, the instruction about to run */
    int page;               /* A14-A13 as an address ($0000, $2000, $4000, $6000) */
    int iar;                /* A12-A0 */
    /* general registers: r[bank][n], n 0-3 and n == 0 is R0 (the same in both
     * banks); r[1][1..3] are R1'-R3'. `rs` says which bank the instructions
     * name. */
    uint8_t r[2][4];
    uint8_t r0;
    int psu, psl;           /* raw status bytes */
    /* PSU */
    int sense;              /* S: the SENSE input (the UVI's vblank) */
    int flag;               /* F: the FLAG output */
    int ii;                 /* II: interrupt inhibit */
    int sp;                 /* SP: return stack pointer, 0-7 */
    /* PSL */
    int cc;                 /* condition code 0 zero, 1 positive, 2 negative */
    int idc, rs, wc, ovf, com, c;
    uint16_t ras[8];        /* return address stack, 15-bit addresses. A call increments
                               SP and stores the return address at ras[SP]: ras[sp] is
                               the top, the live returns are ras[1..sp] (ras[0] is the
                               slot SP = 7 + 1 wraps to) */
    int halted;             /* HALT executed, waiting for an interrupt */
    int irq_line;           /* the IRQ input (the Arcadia never drives it) */
    uint64_t cycles;        /* CPU clocks since reset (6, 9 or 12 per instruction) */
    uint64_t instructions;
    unsigned illegal_count; /* illegal opcodes executed */
    int illegal_pc;         /* the last one's address, -1 if none */
    int vpos, hpos;         /* where the beam is: the UVI's line, and the pixel in it */
    uint32_t frame;
    int machine;            /* ARC_MACHINE_* */
} arcdebug_cpu;
void arcdebug_cpu_get(arcdebug *d, arcdebug_cpu *out);
/* Registers for arcdebug_cpu_set() and the prompt's `set`. */
typedef enum {
    ARC_REG_R0 = 0,
    ARC_REG_R1, ARC_REG_R2, ARC_REG_R3,          /* the bank in use */
    ARC_REG_R1B, ARC_REG_R2B, ARC_REG_R3B,       /* bank 1: R1'-R3' */
    ARC_REG_PC, ARC_REG_IAR,
    ARC_REG_PSU, ARC_REG_PSL,
    ARC_REG_SP, ARC_REG_F, ARC_REG_II,
    ARC_REG_CC, ARC_REG_IDC, ARC_REG_RS, ARC_REG_WC, ARC_REG_OVF, ARC_REG_COM, ARC_REG_C,
    ARC_REG_RAS0,                                /* ... ARC_REG_RAS0 + 7: the return stack */
    ARC_REG_COUNT = ARC_REG_RAS0 + 8
} arcdebug_reg;
/* Only while stopped. PSU/PSL/RS/F take the CPU's side effects (a changed RS
 * swaps the register sets). */
void arcdebug_cpu_set(arcdebug *d, int reg, int value);

/* ---- memory ------------------------------------------------------------------- */
/* The three address spaces. Reads have no side effects (no mailbox hotspot
 * fires, no collision flag clears). */
typedef enum {
    ARC_SPACE_PROG = 0,   /* the CPU's view, 15-bit $0000-$7FFF, as a read would see it */
    ARC_SPACE_UVI,        /* the UVI's register file, $000-$2FF (CPU $1800-$1AFF), direct */
    ARC_SPACE_CART        /* the cartridge's own image: the FujiNet cart's buffer
                             $00000-$117FF (APPBANK page n at $2000 + (n-1)*$800;
                             the mailbox is image $1B00-$1FFF), a mapper cart's ROM */
} arcdebug_space;
/* Size of a space in bytes (the CART space's depends on the cartridge). */
int  arcdebug_space_size(arcdebug *d, int space);
/* Fill dst with n bytes from addr; returns n, or 0 when the machine is not
 * running. Addresses past a space's end read $FF. */
int  arcdebug_read(arcdebug *d, int space, uint32_t addr, uint8_t *dst, int n);
/* A debugger edit: the UVI takes it with its write effects (colour latches,
 * UDCs, object positions) as if the CPU had written it; a PROG address in the
 * UVI window is the same; a PROG address the cartridge serves, or CART, goes
 * into the cartridge's own buffer (the image or the cart RAM), never a
 * hotspot; any other PROG write is ignored (it is ROM). */
void arcdebug_write(arcdebug *d, int space, uint32_t addr, uint8_t value);

/* ---- disassembly -------------------------------------------------------------- */
typedef struct {
    uint16_t address;        /* 15-bit */
    int is_pc;
    int has_breakpoint;
    int length;              /* 1-3 */
    char bytes[16];          /* "0C 18 00" */
    char label[48];
    char disasm[64];         /* MAME's text: "lodi,r1 $12", "bcta,un $0412" */
    char comment[64];        /* a branch target's label */
} arcdebug_line;
/* Up to `max` lines starting at `addr` (PROG space). Returns the count;
 * *pc_line is the index of the PC's line in `out`, -1 when not among them. */
int  arcdebug_disassemble(arcdebug *d, uint16_t addr, arcdebug_line *out,
                              int max, int *pc_line);
/* The address `rows` disassembly rows before (negative) or after `addr` --
 * for scrolling, and for "Follow PC" (PC a third of the way down). Backwards
 * is a best guess. */
int  arcdebug_row_address(arcdebug *d, uint16_t addr, int rows);
/* Address of a label, or -1. Label of an address into dst (may be empty). */
int  arcdebug_label_address(arcdebug *d, const char *label);
int  arcdebug_address_label(arcdebug *d, uint16_t addr, char *dst, int dstsz);
int  arcdebug_set_label(arcdebug *d, uint16_t addr, const char *label);
/* Load a symbol file: Macroassembler AS (-g MAP's .map, or the -L listing's
 * symbol table), WLA-DX .sym, or a plain "AAAA name" list. With path NULL,
 * <cart>.map/.lst/.sym next to the opened cartridge. Built-in labels: the
 * reset vector, the UVI's registers ($1800-$1AFF: SCR1, OBJ0_Y..OBJ3_X, VPOS,
 * SND1, SND2, CHARLINE, PAD1A.., PANEL, UDC38.., PAL0-PAL3, COLL_BG, COLL_OBJ,
 * POT1, POT2, SCR2) and the FujiNet mailbox ($2B00 reply slice, $2C00 status
 * page with the $2C0E/F tick, $2D00 REGSEL, $2D80 APPBANK select, $2DFE
 * SWAP, $2E00 REGDATA, $2F00 DATA). Message into msg; returns labels loaded
 * or -1. */
int  arcdebug_load_symbols(arcdebug *d, const char *path, char *msg, int msgsz);

/* ---- video: the 2637 UVI --------------------------------------------------------
 *
 * The UVI draws one character row per 8 (or 16) beam lines from its registers
 * as they are at that line, so a register changed mid-frame lands on the right
 * line. Three ways to look at the picture, all as XRGB images of
 * ARCSESSION_FB_WIDTH x ARCSESSION_FB_HEIGHT (times `scale`): what the machine
 * last finished (FRAME), the registers drawn afresh as of now (RENDER, which
 * is what the next frame would be if the registers stopped changing; layers
 * and overlays apply), and the frame as far as the beam has got (PROGRESS).
 *
 * Colours are the UVI's 8: 0 white, 1 yellow, 2 cyan, 3 green, 4 magenta, 5 red,
 * 6 blue, 7 black (names from arcdebug_uvi_colour_name). */

typedef struct {
    int index;               /* 0-3 */
    int x, y;                /* the object's position on the screen (0,0 = the
                                top-left of the 128-pixel screen; may be negative
                                or past the edge) */
    int px, py;              /* ... and in frame pixels (x + 32, y): the top-left
                                of its box in the image the views draw */
    uint8_t reg_x, reg_y;    /* the raw position registers */
    int colour;              /* 0-7 */
    int double_height;       /* the size flag: drawn 8 wide, 16 lines tall (each
                                row of the image doubled) */
    int width, height;       /* its box on screen in frame pixels: 8 x (8 | 16) */
    int udc;                 /* the user-defined character holding its image: $38 + index */
    int visible;             /* geometric: inside what the chip draws */
    uint8_t shape[8];        /* the image, one byte a row, bit 7 leftmost */
    int hits_chars;          /* has hit the character screen since the last frame */
    int hits_objects;        /* mask of the other objects it overlaps (bit n = object n) */
} arcdebug_uvi_object;

typedef struct {
    uint8_t reg[0x300];                  /* the register file, raw ($1800-$1AFF) */
    arcdebug_uvi_object obj[4];
    /* the beam and the character screen's geometry */
    int line;                            /* the beam line (MAME's m_line) */
    int charline;                        /* the character row the beam is in ($18FF) */
    int vblank;                          /* SENSE: line >= 216 */
    int first_line;                      /* first line of the character screen ($18FC) */
    int shift;                           /* fine scroll, 0-7 pixels ($18FE bits 5-7) */
    int rows;                            /* character rows shown: 13, or 26 when two screens */
    int cell_height;                     /* 8, or 16 (8x16 characters) */
    int two_screens;                     /* $19F8 bit 6: the lower screen $1A00 shown */
    int block_graphics;                  /* $19F8 bit 7 at the top of the screen */
    int multicolor;                      /* $18FD bit 7: colour bits per character */
    int pot_y;                           /* $19F9 bit 6 clear: the pots read the other axis (mux) */
    /* colours */
    int screen_colour, char_colour;      /* $19F9 */
    int alt_screen_colour, alt_char_colour;  /* $19F8 */
    /* collisions as the CPU reads them: $19FC (objects x characters, bit n = object n,
     * a 0 bit is a hit) and $19FD (object pairs, 0 = hit: b0 0-1, b1 0-2, b2 0-3, b3 1-2,
     * b4 1-3, b5 2-3) */
    uint8_t coll_chars, coll_objects;
    /* the beeper (the UVI's two sound registers) */
    int sound_pitch;                     /* $18FD bits 0-6: the divider */
    int sound_volume;                    /* $18FE bits 0-2 */
    int sound_tone, sound_noise;         /* $18FE bits 3, 4 */
    /* pots and keypads as the CPU would read them now */
    uint8_t pad[8];                      /* $1900-$1907 */
    uint8_t panel;                       /* $1908: b0 START, b1 OPTION, b2 SELECT (1 = held) */
    uint8_t pot[2];                      /* $19FF (stick 1), $19FE (stick 2) */
    uint8_t chars[0x40][8];              /* the glyph table: $00-$37 the built-in set,
                                            $38-$3F the UDCs ($1980-$19BF) */
    int lines;                           /* lines per frame: 262 (NTSC) / 312 (PAL) */
} arcdebug_uvi;
/* Everything the chip knows, decoded. */
void arcdebug_uvi_get(arcdebug *d, arcdebug_uvi *out);

/* One cell of the character screen as the chip draws it. */
typedef struct {
    uint8_t code;            /* the byte in the screen RAM */
    uint8_t glyph;           /* code & $3F: the glyph, $38-$3F being the UDCs */
    uint8_t graphics;        /* drawn as a block-graphics cell (2 x 3 blocks per bit) */
    uint8_t fg, bg;          /* the colours it is drawn in, 0-7 (the character's own
                                colour bits in multicolor mode, the screen's otherwise) */
} arcdebug_uvi_cell;
#define ARCDEBUG_UVI_COLS 16
#define ARCDEBUG_UVI_MAX_ROWS 26
/* The character screen, row-major, 16 columns: fills ARCDEBUG_UVI_COLS * rows
 * cells (rows is 13, or 26 with two screens) and returns rows; 0 when the
 * machine is not running. cells must hold ARCDEBUG_UVI_COLS * ARCDEBUG_UVI_MAX_ROWS.
 * Block graphics follow the chip: on from the top of the screen when $19F8
 * bit 7 is set, switched on by a $C0 code and off by a $40 code within a row. */
int  arcdebug_uvi_cells(arcdebug *d, arcdebug_uvi_cell *cells);

/* A register's name and a decoded value line (live): "OBJ1_X", "x=17". The
 * decoded text is empty for plain RAM. offset is 0-$2FF (CPU address - $1800). */
void arcdebug_uvi_describe(arcdebug *d, unsigned offset, char *name, size_t name_n,
                           char *text, size_t text_n);
/* A debugger store into the UVI with the chip's write effects (offset 0-$2FF);
 * the machine should be stopped. Same as arcdebug_write(ARC_SPACE_UVI, ...). */
void arcdebug_uvi_write(arcdebug *d, unsigned offset, uint8_t value);
/* The name of a UVI colour 0-7. */
const char *arcdebug_uvi_colour_name(int colour);

/* Layers for RENDER (and what the sheets draw), and overlays for any of the
 * three picture views. */
#define ARCDEBUG_UVI_L_CHARS    0x01u    /* the character screen */
#define ARCDEBUG_UVI_L_OBJECTS  0x02u    /* the four objects */
#define ARCDEBUG_UVI_L_ALL      0x03u
#define ARCDEBUG_UVI_F_BOXES    0x01u    /* an outline round each visible object (red, green, blue, orange for 0-3) */
#define ARCDEBUG_UVI_F_GRID     0x02u    /* the character cells' borders */
#define ARCDEBUG_UVI_F_BEAM     0x04u    /* a cross-hair at the beam */
#define ARCDEBUG_UVI_F_COLLIDE  0x08u    /* objects that hit something outlined in magenta (replaces the box) */

typedef enum {
    ARCDEBUG_UVI_FRAME = 0,   /* the last frame the machine finished (what the display shows) */
    ARCDEBUG_UVI_RENDER,      /* the CURRENT registers re-drawn with `layers` and `flags`
                                 (works while stopped, whatever the beam is doing) */
    ARCDEBUG_UVI_PROGRESS,    /* the frame as far as the beam has got, the lines the beam
                                 has not reached yet dimmed */
    ARCDEBUG_UVI_FONT,        /* the glyph table as 64 glyphs, 16 x 4, in the character
                                 colours (the UDCs are $38-$3F, the last 8) */
    ARCDEBUG_UVI_OBJECTS,     /* the four objects' images side by side, in their colours,
                                 8 x 16 cells (double-height ones tall) */
    ARCDEBUG_UVI_PALETTE      /* the 8 colours, one cell each in a row */
} arcdebug_uvi_view_kind;
#define ARCDEBUG_UVI_VIEW_MAX_PIXELS (ARCSESSION_FB_WIDTH * ARCSESSION_FB_HEIGHT)
/* The views as XRGB images (every one ends with the pixel size in *w,*h; dst
 * NULL asks for the size only). `scale` is whole screen pixels per image
 * pixel (1 and up; the picture views are ARCSESSION_FB_WIDTH x
 * ARCSESSION_FB_HEIGHT times it, so size dst by ARCDEBUG_UVI_VIEW_MAX_PIXELS *
 * scale * scale). layers and flags apply to RENDER; flags also to FRAME and
 * PROGRESS (overlays are drawn on the picture before it is scaled). Returns
 * 1 when drawn. */
int  arcdebug_uvi_view(arcdebug *d, int view, unsigned layers, unsigned flags,
                       int scale, uint32_t *dst, size_t cap, int *w, int *h);

/* One thing as the chip draws it, for the tables: kind ARCDEBUG_UVI_OBJECT
 * (index 0-3: its image in its colour, 8 x 8 or 8 x 16 times scale) or
 * ARCDEBUG_UVI_GLYPH (index 0-63: the glyph in the character colours, 8 x 8
 * times scale). Pixels not set take `bg` (XRGB). */
#define ARCDEBUG_UVI_OBJECT 0
#define ARCDEBUG_UVI_GLYPH  1
int  arcdebug_uvi_thing(arcdebug *d, int kind, int index, int scale, uint32_t bg,
                        uint32_t *dst, size_t cap, int *w, int *h);

/* What draws at a pixel of the ARCSESSION_FB_WIDTH x ARCSESSION_FB_HEIGHT
 * frame, topmost first, written into out[] (up to max entries; the count is
 * returned): kind ARCDEBUG_UVI_HIT_OBJECT with index 0-3 (only where one of
 * its set bits is under the pixel), then ARCDEBUG_UVI_HIT_CELL with index
 * row * 16 + column of the character cell (13 or 26 rows) when the pixel is
 * in the 128-pixel screen. */
#define ARCDEBUG_UVI_HIT_OBJECT 0
#define ARCDEBUG_UVI_HIT_CELL   1
typedef struct { int kind, index; } arcdebug_hit;
int  arcdebug_uvi_hit_test(arcdebug *d, int x, int y, arcdebug_hit *out, int max);

/* ---- inputs and the console ------------------------------------------------------ */
typedef struct {
    int machine;                         /* ARC_MACHINE_* */
    int is_pal;
    uint64_t keys;                       /* the matrix as the machine has latched it
                                            (bit 0-11 pad 1, 12-23 pad 2, 24-26 panel;
                                            ARC_KEY order in arcsession.h) */
    uint8_t sticks[2];                   /* bit 0 UP, 1 RIGHT, 2 DOWN, 3 LEFT, 4 FIRE */
    int8_t  disc[2][2];                  /* analog -128..127, [stick][x, y]; 0 = digital */
    uint8_t pad[8];                      /* the keypad columns the CPU reads ($1900-$1907) */
    uint8_t panel;                       /* $1908 */
    uint8_t pot[2][2];                   /* the four pot values [stick][x, y] */
    int reset_held;                      /* the console's RESET key (the 2650's /RESET) */
    int sense;                           /* SENSE (vblank) */
    int flag;                            /* the FLAG output */
    int cart_type;                       /* enum arc_cart_type: 1 standard, 2 Golf, 3 FujiNet */
    uint64_t audio_samples;              /* produced since power-on */
} arcdebug_io;
void arcdebug_io_get(arcdebug *d, arcdebug_io *out);

/* ---- the cartridge ------------------------------------------------------------ */
typedef struct {
    int present;
    int type;                            /* 0 none, 1 standard, 2 Golf, 3 FujiNet */
    const char *type_name;               /* "FujiNet", "Standard", "Golf", ... */
    /* a mapper cartridge */
    uint32_t rom_size;
    /* the FujiNet cartridge (type FujiNet) */
    int link_up, link_off, mailbox, app_live, booted_game, armed;
    int staged_app, staged_mailbox;
    uint32_t live_size, live_crc, staged_size, staged_crc;
    int swaps;
    int nbanks, fbank;                   /* APPBANK pages (incl. page 0), and the one shown at $0800 */
    int boot_state, boot_pct, boot_err;
    uint8_t ackseq, status, last_error, reply_cmd;
    uint16_t rxlen;
    uint32_t queue_depth;
    uint8_t mailbox_page[0x500];         /* $2B00-$2FFF as the console reads it (side-effect free) */
} arcdebug_cart;
void arcdebug_cart_get(arcdebug *d, arcdebug_cart *out);
/* One line: "FujiNet cartridge: CONFIG, link up". */
int  arcdebug_cart_info(arcdebug *d, char *dst, int dstsz);

/* ---- FujiNet: the cartridge's socket, as FujiBus packets ------------------------ */
typedef struct {
    uint64_t serial;
    int      to_host;       /* 1: FujiNet -> Arcadia 2001, 0: Arcadia 2001 -> FujiNet */
    int      device, command, length;   /* from the FujiBus header, -1 if short */
    int      nbytes;
    uint8_t  data[64];      /* the decoded packet's first bytes */
    char     text[96];      /* "Arcadia 2001 -> FujiNet 70 F4 READ HOST SLOTS (6 bytes)" */
    uint32_t repeat;        /* the same exchange back to back, shown "xN" */
} arcdebug_fuji_packet;
/* Recorded from session start, attached or not. The newest `max`, oldest
 * first; returns the count. */
int  arcdebug_fujinet_trace(arcdebug *d, arcdebug_fuji_packet *out, int max);
void arcdebug_fujinet_trace_clear(arcdebug *d);
/* The cartridge's link and mailbox, as text lines. Returns length. */
int  arcdebug_fujinet_state(arcdebug *d, char *dst, int dstsz);

/* ---- breakpoints -------------------------------------------------------------- */
typedef enum {
    ARCDEBUG_BP_EXEC = 1,    /* the instruction at the address is about to run */
    ARCDEBUG_BP_READ = 2,    /* a data read of PROG space (not an instruction fetch) */
    ARCDEBUG_BP_WRITE = 4    /* a write to PROG space: the UVI at $1800-$1AFF is the only target */
} arcdebug_bp_type;
typedef struct {
    int id;
    int type;                /* arcdebug_bp_type bits */
    int space;               /* always ARC_SPACE_PROG */
    uint16_t start, end;     /* a 15-bit address range */
    int enabled;
    unsigned hits;
    char condition[128];     /* may use value, addr, registers, flags, vpos, hpos, labels, p[] u[] c[] */
} arcdebug_breakpoint;
/* Execute breakpoint at addr: added if absent, removed if present. Returns
 * 1 when one is now set. */
int  arcdebug_breakpoint_toggle(arcdebug *d, uint16_t addr);
int  arcdebug_breakpoint_check(arcdebug *d, uint16_t addr);
/* Returns the new id, or -1 if the condition does not parse, the type is
 * invalid or the range is outside PROG space. */
int  arcdebug_breakpoint_add(arcdebug *d, int type, int space, uint16_t start,
                            uint16_t end, const char *condition);
void arcdebug_breakpoint_remove(arcdebug *d, int id);
void arcdebug_breakpoint_enable(arcdebug *d, int id, int enabled);
int  arcdebug_breakpoint_list(arcdebug *d, arcdebug_breakpoint *out, int max);
void arcdebug_breakpoint_clear(arcdebug *d);

/* ---- the trace ring ----------------------------------------------------------- */
typedef struct {
    uint16_t pc;
    uint8_t r0, r1, r2, r3;  /* the set in use */
    uint8_t psu, psl;
    uint8_t len, bytes[3];
    uint64_t cycles;
    int16_t vpos, hpos;
} arcdebug_trace;
void arcdebug_trace_enable(arcdebug *d, int on);
int  arcdebug_trace_enabled(arcdebug *d);
/* Newest first; returns entries copied. */
int  arcdebug_trace_read(arcdebug *d, arcdebug_trace *out, int max);
void arcdebug_trace_clear(arcdebug *d);

/* ---- files -------------------------------------------------------------------- */
/* Save with the path a native file picker supplied. kind: "dis" (PROG space
 * $0000-$7FFF as code, as mapped now), "mailbox" (the cart's $2B00-$2FFF
 * page), "uvi" (the 768 register bytes), "font" (the glyph table, 64 x 8
 * bytes), "cart" (the cartridge image), "screen" (the frame as a PPM), "regs"
 * (CPU, UVI and I/O state as text). Message into msg; returns 0 or -1. */
int  arcdebug_save(arcdebug *d, const char *kind, const char *path,
                       char *msg, int msgsz);

#ifdef __cplusplus
}
#endif

#endif /* ARCDEBUG_H */
