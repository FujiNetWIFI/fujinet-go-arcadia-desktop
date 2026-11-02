/*
 * arcsession -- the toolkit-agnostic desktop session for FujiNet Go Arcadia 2001.
 *
 * Owns the emulator (the Arcadia 2001 / Videopac core on its own paced thread --
 * see core/arc/host.h), the FujiNet cartridge, the SDL audio and gamepad
 * backends, the in-process FujiNet runtime, the shared settings store, the
 * remappable key/pad bindings, the imported ROMs and the media path layout.
 * Frontends (GTK4, Qt6, AppKit, Win32) drive this API and do only windowing,
 * painting and event translation. A frontend that needs something which is
 * not one of those three things belongs here instead.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARCSESSION_H
#define ARCSESSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MAME's visible area: 360 x 243 (the 8244/8245 VDC's picture; the G7400's
 * EF9340 text layer is composited into the same frame). Frontends show it at
 * the TV's 4:3 or with square pixels, per the "aspect" setting. */
#define ARCSESSION_FB_WIDTH  360
#define ARCSESSION_FB_HEIGHT 243

/* FujiNet's BoIP listener and its web admin UI. High ports of this app's own
 * so a standalone fujinet-pc, or a sibling FujiNet Go app, never collides:
 * ADAM uses 65216/65214, Apple II 1985/64001, CoCo 65504/64002,
 * Intellivision 65503/64003, MSX 65505/64003, Astrocade 11500/11501,
 * ColecoVision 11502/11503, Atari 2600 11504/11505, NES 11506/11507, SMS
 * 11508/11509, Atari 7800 11510/11511, H89 11512/11513, Atari 800
 * 11514/11515, MS-DOS 11516/11517, Studio II 11518/11519; the firmware's
 * MAME dev harness for the Arcadia 2001 uses 9995/9997.
 *
 * Direction: FujiNet LISTENS and the Arcadia 2001 FujiNet cartridge dials in, as
 * on every sibling. That decides startup ordering -- see arcsession_start. */
#define ARCSESSION_BOIP_PORT  11520
#define ARCSESSION_WEBUI_PORT 11521

/* The host audio device rate; the core streams the 8244's sound and The
 * Voice's speech mixed at it. */
#define ARCSESSION_AUDIO_RATE 48000

/* The accent colour every frontend uses for its highlights (the Map target,
 * held keys, the debugger's current line, the FujiNet status dot): the
 * Arcadia 2001's power-button red, as on the console. White text sits on it; on
 * the light themes a frontend that draws text in the accent uses the darker
 * variant (the console's reset key). */
#define ARCSESSION_ACCENT_RGB      0xC11218
#define ARCSESSION_ACCENT_DARK_RGB 0x942324

/* The machines. Arcadia 2001: NTSC, 8244. Videopac G7000: PAL, 8245, the same
 * BIOS. Videopac+ G7400: PAL, 8245 + EF9340/41 text chip + 8243, its own
 * BIOS and a larger keyboard. Setting "machine". */
typedef enum {
    ARC_MACHINE_ARCADIA = 0,
    ARC_MACHINE_VIDEOPAC,
    ARC_MACHINE_VIDEOPAC_PLUS,
    ARC_MACHINE_COUNT
} arc_machine;
const char *arc_machine_name(int machine);        /* "Magnavox Arcadia 2001 (NTSC)" */
int arc_machine_is_pal(int machine);

typedef struct arcsession arcsession;
typedef struct arcdebug arcdebug;

/* All members optional (NULL = default).
 *  config_dir:  default $XDG_CONFIG_HOME/fujinet-go-arcadia
 *  data_dir:    default $XDG_DATA_HOME/fujinet-go-arcadia
 *  fujinet_lib: path to libfujinet.so/.dylib/.dll; default searches
 *               $FUJINET_LIB, the executable's directory, the install
 *               libdir, then tools/fujinet/work/out. "" disables FujiNet.
 *  fujinet_runtime_src: directory holding the pristine fnconfig.ini + data/
 *               + SD/ used to provision the user's runtime tree on first
 *               start (a macOS app passes its bundle's runtime dir). */
typedef struct {
    const char *config_dir;
    const char *data_dir;
    const char *fujinet_lib;
    const char *fujinet_runtime_src;
} arcsession_paths;

arcsession *arcsession_new(const arcsession_paths *paths);
void arcsession_free(arcsession *s);

/* ---- settings (shared INI; one store for every frontend of this target) --- */
int         arcsession_get_int(arcsession *s, const char *key, int def);
void        arcsession_set_int(arcsession *s, const char *key, int value);
const char *arcsession_get_str(arcsession *s, const char *key,
                               const char *def);
void        arcsession_set_str(arcsession *s, const char *key,
                               const char *value);
void        arcsession_settings_flush(arcsession *s);

/* ---- lifecycle ------------------------------------------------------------ */
typedef struct {
    const char *cart_path;    /* cartridge image; NULL boots the CONFIG client */
    int machine;              /* arc_machine */
    int use_bios;             /* run the imported BIOS when there is one (else
                                 the stand-in, which runs FujiNet apps only) */
    int voice;                /* The Voice in front of the cartridge */
    int analog_joystick;      /* gamepad sticks drive the directions too */
    int enable_fujinet;       /* start the in-process FujiNet runtime */
    int external_fujinet;     /* 1: don't start it; dial another FujiNet
                               * instance at boip_connect instead */
    const char *boip_connect; /* "host:port" or "port" the cartridge dials
                               * (external only); NULL/"" = 127.0.0.1:11520 */
    int enable_audio;         /* open the SDL audio device */
    int enable_gamepad;       /* start the SDL gamepad thread */
} arcsession_start_opts;

/* Fills opts from the settings store (keys: cart machine use_bios voice
 * analog_joystick enable_fujinet enable_audio enable_gamepad
 * fujinet_external fujinet_external_addr). */
void arcsession_default_opts(arcsession *s, arcsession_start_opts *opts);

/* Starts FujiNet (if enabled) and then the machine.
 *
 * The order is not arbitrary: FujiNet listens and the cartridge dials in, so
 * the listener has to exist before the machine's first transaction or the
 * CONFIG client boots reporting no link. start() brings FujiNet up first and
 * waits briefly for the port.
 *
 * Returns 0, or -1 with arcsession_last_error() set. FujiNet failing to
 * start is NOT fatal: the machine boots with the cartridge reporting the
 * link down, which is far more useful than refusing to run. */
int  arcsession_start(arcsession *s, const arcsession_start_opts *opts);
void arcsession_stop(arcsession *s);
int  arcsession_is_running(const arcsession *s);
const char *arcsession_last_error(const arcsession *s);
/* Power-cycle with the current settings (stop + default_opts + start):
 * what Preferences does after a machine option changes. */
int  arcsession_restart(arcsession *s);

/* ---- cartridges ------------------------------------------------------------
 * The FujiNet cartridge is always in the slot; CONFIG is its resident image.
 *
 * load_cart puts a local image (.bin, .rom or .arc) into the cartridge in
 * place of CONFIG and power-cycles the console onto it, as the firmware's MAME
 * device serves its -cart image: a FujiNet app (the "FUJI" claim at $F2C of an
 * APPBANK image) keeps the mailbox, a game runs alone behind the FujiNet slot
 * with the right mapper (standard, 4-in-1, rally, KTAA, by CRC as MAME's
 * software list does). An image the cartridge cannot plan is refused with
 * the reason; so is a game while no BIOS is imported (commercial games call
 * the BIOS: only the real one runs them), and the Chess and Home Computer
 * modules (their own CPUs). The path is remembered in the "cart" setting, so
 * it boots again next start. Returns 0 or -1 + error. */
int  arcsession_load_cart(arcsession *s, const char *path);
const char *arcsession_cart_path(const arcsession *s);
/* 1 if the image can run; else 0 and why. */
int  arcsession_check_cart(const char *path, char *why, int whysz);
/* 1 if the image is a FujiNet app (runs without the BIOS). */
int  arcsession_cart_is_app(const char *path);
/* A description of the cartridge now running ("Standard, 8 KB",
 * "FujiNet CONFIG", "4-in-1", ...); returns the length. */
int  arcsession_cart_type_name(arcsession *s, char *dst, int dstsz);

/* The console's red RESET key: it resets the 8048 only and never reaches the
 * cartridge, so a running FujiNet app restarts from the cart's current image.
 * Pressed and released for a few frames (the controllers window and the
 * menus); the bound key (F1, MAME's) holds it for as long as it is down. */
int  arcsession_reset(arcsession *s);
/* Reset to CONFIG: a power cycle. Ejects an opened cartridge (clears the
 * "cart" setting) and boots CONFIG, exactly as at power-on -- on the
 * hardware, only taking the console's power away brings CONFIG back.
 * Escape by default. Every power cycle (this, Eject, Open Cartridge) takes
 * the BIOS as the settings name it now, so an imported BIOS boots here. */
int  arcsession_reset_to_config(arcsession *s);
/* Eject: the same as Reset to CONFIG (there is no "no cartridge" state
 * worth having on a FujiNet cartridge). */
int  arcsession_eject(arcsession *s);
/* What the running configuration is missing, as a mask of ARC_MISSING_*: the
 * stand-in met a game it cannot run (no BIOS), the 8244's character set is not
 * imported (text shows blank), a Voice game or the Voice switch has no speech
 * ROM. The frontends say "Import BIOS" and name the part. Cleared by the next
 * power cycle that fixes it. */
#define ARC_MISSING_BIOS   0x01   /* a game needs the console BIOS */
#define ARC_MISSING_FONT   0x02   /* the 8244/8245 (or EF9340) character set */
#define ARC_MISSING_VOICE  0x04   /* The Voice's speech ROMs */
unsigned arcsession_missing(arcsession *s);
/* Same as arcsession_missing() & ARC_MISSING_BIOS. */
int  arcsession_needs_bios(arcsession *s);
/* Pause/resume the machine (the debugger's Run/Stop is separate). */
void arcsession_set_paused(arcsession *s, int paused);
int  arcsession_is_paused(const arcsession *s);

/* ---- video ---------------------------------------------------------------
 * copy_frame copies the latest frame into dst (FB_WIDTH*FB_HEIGHT uint32
 * XRGB8888 pixels) iff its serial differs from *serial_inout, updates it and
 * returns 1; returns 0 when unchanged, leaving dst alone. Pass 0 to force a
 * copy. */
int  arcsession_copy_frame(arcsession *s, uint32_t *dst, uint64_t *serial_inout);
/* Ask to be told when a new frame is ready, for a frontend with no frame
 * clock of its own: cb(ctx) runs on the machine thread after each frame is
 * published (a frame identical to the previous one is not published again).
 * Keep it quick (post an event and return) and do not call into the session
 * from it. Passing NULL stops the calls, and returns only after any call in
 * flight has finished, so ctx may be freed right after. */
typedef void (*arcsession_frame_cb)(void *ctx);
void arcsession_set_frame_callback(arcsession *s, arcsession_frame_cb cb, void *ctx);
/* Frames the machine has run since it started. Unlike copy_frame's serial it
 * counts every frame, including the ones identical to their predecessor:
 * use it to ask "is the machine running", not "is the picture changing". */
uint64_t arcsession_frame_count(arcsession *s);
/* The machine's refresh rate, rounded (NTSC 60: 59.83 Hz; PAL 50: 49.70 Hz). */
int  arcsession_refresh_rate(arcsession *s);

/* Feed the UI's frame-clock ticks (CLOCK_MONOTONIC ns). While a steady
 * stream near the console's refresh rate arrives, the machine phase-locks
 * one frame per tick; otherwise it paces on the wall clock. A frontend with
 * no frame clock simply never calls this. */
void arcsession_notify_vsync(arcsession *s, int64_t frame_time_ns);

/* ---- audio ---------------------------------------------------------------
 * Owned by the session (SDL) when opts.enable_audio was set. A frontend
 * that wants the device itself can pull interleaved stereo float frames at
 * ARCSESSION_AUDIO_RATE instead. Returns the frames written (silence is
 * written for any shortfall). */
int  arcsession_render_audio(arcsession *s, float *out, int nframes);
void arcsession_set_volume(arcsession *s, int percent);

/* ---- input ---------------------------------------------------------------
 * Every control the machine has, flattened into one target index so the
 * bindings table, the settings store and the frontends can all name them:
 * per stick (1, then 2) its four directions and the button, then the
 * keyboard's 64 matrix positions (row * 8 + column; the Arcadia 2001 uses
 * 0-47, the Videopac+ G7400 adds rows 6 and 7), then the console's RESET
 * key, then the session's own actions. The console reads the matrix one row
 * at a time, so any number of keys may be held. */
typedef enum {
    ARC_ACT_UP = 0, ARC_ACT_RIGHT, ARC_ACT_DOWN, ARC_ACT_LEFT, ARC_ACT_FIRE,
    ARC_ACT_PER_PORT
} arc_action;

#define ARC_KEY_COUNT 64

typedef enum {
    ARC_SW_RESET = 0,                 /* F1 by default (MAME's) */
    ARC_SW_COUNT
} arc_switch;

typedef enum {
    ARC_SYSACT_RESET_CONFIG = 0,      /* Escape by default */
    ARC_SYSACT_DEBUG_STOP,            /* stop in the debugger */
    ARC_SYSACT_COUNT
} arc_sysaction;

#define ARC_TARGET_PORT(port, act) ((port) * ARC_ACT_PER_PORT + (act))
#define ARC_TARGET_KEY(key)        (2 * ARC_ACT_PER_PORT + (key))
#define ARC_TARGET_SWITCH(sw)      (2 * ARC_ACT_PER_PORT + ARC_KEY_COUNT + (sw))
#define ARC_TARGET_SYSACT(sa)      (2 * ARC_ACT_PER_PORT + ARC_KEY_COUNT + ARC_SW_COUNT + (sa))
#define ARC_TARGET_COUNT           (2 * ARC_ACT_PER_PORT + ARC_KEY_COUNT + ARC_SW_COUNT + ARC_SYSACT_COUNT)

/* Apply one control directly. `down` is press/release. The matrix and the
 * sticks are sampled by the console at the frame boundary, as MAME's input
 * ports are. */
void arcsession_press(arcsession *s, int target, int down);
void arcsession_sysaction(arcsession *s, int sysact);
/* Release everything the keyboard holds (focus loss). */
void arcsession_release_all(arcsession *s);
/* What the console sees held on a stick right now (keyboard and gamepads
 * together), one bit per arc_action. For the controllers window's highlights. */
unsigned arcsession_buttons_held(arcsession *s, int port);
/* Whether matrix key `key` (0-63) is held, from any source. */
int arcsession_key_held(arcsession *s, int key);
/* Whether a console button is held (arc_switch). */
int arcsession_switch_held(arcsession *s, int sw);

/* ---- the on-screen keyboard -----------------------------------------------
 * The membrane keyboard as it is printed on the console (the Arcadia 2001's, or
 * the Videopac+ G7400's when that machine is selected): one entry per key cap,
 * in key-cap units (1.0 = a letter key), x/y/w/h from the top-left of the
 * panel. `target` is the matrix position (a binding target is
 * ARC_TARGET_KEY(target)); -1 marks the RESET key, which is ARC_SW_RESET.
 * YES and NO are separate caps that press the same switches as Y and N (the
 * console wires each pair to one matrix position). `kind` says how a toolkit
 * should paint it: normal, function (red legend), input, reset, space. The
 * table is static; the pointer stays valid for the process. */
typedef enum {
    ARC_KEYCAP_NORMAL = 0, ARC_KEYCAP_FUNCTION, ARC_KEYCAP_INPUT,
    ARC_KEYCAP_RESET, ARC_KEYCAP_SPACE
} arc_keycap_kind;

typedef struct {
    const char *label;       /* "Q", "+", "\xc3\x97" (x), "YES", "ENTER", "SPACE" */
    int target;              /* matrix position 0-63, or -1 for RESET */
    float x, y, w, h;
    int kind;                /* arc_keycap_kind */
} arc_keycap;

/* The layout for a machine; *n is its entry count; width/height (key units)
 * are the panel's extent so a toolkit can size its drawing area. */
const arc_keycap *arcsession_keyboard_layout(int machine, int *n, float *width, float *height);
/* The label a matrix position has ("Q", "ENTER", "Y / YES"), or NULL. */
const char *arcsession_key_label(int machine, int key);

/* ---- keyboard translation ------------------------------------------------
 * keysym is an X11/xkb keysym (== a GDK keyval; Qt, Win32 and AppKit map
 * through the HID tables below), so one bindings table serves every
 * frontend. Returns 1 if the key drives a target (and has been applied /
 * released), 0 if it should be ignored. System actions are reported through
 * arcsession_key_sysaction instead and left to the frontend. */
int  arcsession_key(arcsession *s, uint32_t keysym, int down);
/* The system action a keysym is bound to, or -1. */
int  arcsession_key_sysaction(arcsession *s, uint32_t keysym);

/* Non-printing keys in the keysym space the frontends translate to. */
enum {
    ARC_KEYSYM_NONE = 0,
    ARC_KEYSYM_UP = 0xff52, ARC_KEYSYM_DOWN = 0xff54,
    ARC_KEYSYM_LEFT = 0xff51, ARC_KEYSYM_RIGHT = 0xff53,
    ARC_KEYSYM_ESCAPE = 0xff1b, ARC_KEYSYM_RETURN = 0xff0d,
    ARC_KEYSYM_BACKSPACE = 0xff08, ARC_KEYSYM_TAB = 0xff09,
    ARC_KEYSYM_SPACE = 0x20,
    ARC_KEYSYM_F1 = 0xffbe, ARC_KEYSYM_F2, ARC_KEYSYM_F3, ARC_KEYSYM_F4,
    ARC_KEYSYM_F5, ARC_KEYSYM_F6, ARC_KEYSYM_F7, ARC_KEYSYM_F8,
    ARC_KEYSYM_F9, ARC_KEYSYM_F10, ARC_KEYSYM_F11, ARC_KEYSYM_F12,
    ARC_KEYSYM_LSHIFT = 0xffe1, ARC_KEYSYM_RSHIFT = 0xffe2,
    ARC_KEYSYM_LCTRL = 0xffe3, ARC_KEYSYM_RCTRL = 0xffe4,
    ARC_KEYSYM_LALT = 0xffe9, ARC_KEYSYM_RALT = 0xffea,
    ARC_KEYSYM_KP_0 = 0xffb0, ARC_KEYSYM_KP_1, ARC_KEYSYM_KP_2,
    ARC_KEYSYM_KP_3, ARC_KEYSYM_KP_4, ARC_KEYSYM_KP_5, ARC_KEYSYM_KP_6,
    ARC_KEYSYM_KP_7, ARC_KEYSYM_KP_8, ARC_KEYSYM_KP_9,
    ARC_KEYSYM_KP_ENTER = 0xff8d, ARC_KEYSYM_KP_MULTIPLY = 0xffaa,
    ARC_KEYSYM_KP_DIVIDE = 0xffaf, ARC_KEYSYM_KP_PERIOD = 0xffae
};
/* Native key codes for the platforms whose toolkits do not deliver keysyms:
 * Windows scan code (set 1, with the E0 flag), Linux evdev code (GTK/Qt
 * keycode minus 8) and macOS virtual key code, each mapped to a keysym. 0
 * when unknown. */
uint32_t arcsession_keysym_from_win_scancode(unsigned scancode, int extended);
uint32_t arcsession_keysym_from_evdev(unsigned code);
uint32_t arcsession_keysym_from_macos_keycode(unsigned keycode);
/* Name for a keysym ("F1", "Space", "a", "Keypad 5"); returns length. */
int arcsession_keysym_name(uint32_t keysym, char *dst, int dstsz);

/* ---- remappable bindings --------------------------------------------------
 * Every target can be driven by one keyboard key and one gamepad button.
 * Rebinding STEALS: a key drives exactly one target, because one keystroke
 * doing two things is worse than losing the old binding. Persisted in the
 * settings store under "bindings" as only the entries that differ from the
 * defaults. */
typedef enum {
    ARC_PAD_BTN_NONE = -1,
    ARC_PAD_BTN_SOUTH = 0, ARC_PAD_BTN_EAST, ARC_PAD_BTN_WEST,
    ARC_PAD_BTN_NORTH, ARC_PAD_BTN_BACK, ARC_PAD_BTN_GUIDE,
    ARC_PAD_BTN_START, ARC_PAD_BTN_LEFT_STICK, ARC_PAD_BTN_RIGHT_STICK,
    ARC_PAD_BTN_LEFT_SHOULDER, ARC_PAD_BTN_RIGHT_SHOULDER,
    ARC_PAD_BTN_DPAD_UP, ARC_PAD_BTN_DPAD_DOWN, ARC_PAD_BTN_DPAD_LEFT,
    ARC_PAD_BTN_DPAD_RIGHT,
    ARC_PAD_BTN_LEFT_TRIGGER, ARC_PAD_BTN_RIGHT_TRIGGER,
    ARC_PAD_BTN_COUNT,
    /* Raw joystick bands for devices SDL has no gamepad mapping for (a
     * plain HID adapter): button index, and hat directions (hat*8 + dir). */
    ARC_PAD_BTN_RAW_BASE = 64, ARC_PAD_BTN_RAW_LAST = 127,
    ARC_PAD_HAT_BASE = 192, ARC_PAD_HAT_LAST = 255
} arc_pad_button;
#define ARC_PAD_HAT_DIRS 8
#define ARC_PAD_BTN_IS_NAMED(b) ((b) >= 0 && (b) < ARC_PAD_BTN_COUNT)
#define ARC_PAD_BTN_IS_RAW(b)   ((b) >= ARC_PAD_BTN_RAW_BASE && (b) <= ARC_PAD_BTN_RAW_LAST)
#define ARC_PAD_BTN_IS_HAT(b)   ((b) >= ARC_PAD_HAT_BASE && (b) <= ARC_PAD_HAT_LAST)

typedef struct {
    uint32_t keysym;     /* 0 = no key */
    int      button;     /* arc_pad_button, NONE = no gamepad button */
} arc_binding;

const char *arc_target_name(int target);           /* "Player 1: Up" */
const char *arc_target_short_name(int target);     /* "Up" */
arc_binding arcsession_binding_get(arcsession *s, int target);
/* Bind; the previous holder of the key/button (if any) is described into
 * `stolen` (may be NULL). keysym 0 / button NONE unbinds. */
void arcsession_binding_set_key(arcsession *s, int target, uint32_t keysym,
                                char *stolen, int stolensz);
void arcsession_binding_set_button(arcsession *s, int target, int button,
                                   char *stolen, int stolensz);
void arcsession_bindings_reset(arcsession *s);
const char *arc_pad_button_name(int button);
/* The target a keysym / pad button drives, or -1. */
int arcsession_target_for_key(arcsession *s, uint32_t keysym);
int arcsession_target_for_button(arcsession *s, int port, int button);

/* Map mode: after begin(), the next gamepad button pressed on any pad is
 * reported by poll() (returns 1 and the button). cancel() disarms. */
void arcsession_gamepad_capture_begin(arcsession *s);
void arcsession_gamepad_capture_cancel(arcsession *s);
int  arcsession_gamepad_capture_poll(arcsession *s, int *button);

void arcsession_set_analog(arcsession *s, int joystick);

/* ---- gamepads (SDL, hotplugged; started by arcsession_start) -------------
 * Pads are assigned to ports in connection order unless assigned
 * explicitly. A pad that disconnects and reconnects gets its port back. */
int  arcsession_gamepad_count(arcsession *s);
int  arcsession_gamepad_name(arcsession *s, int idx, char *dst, int dstsz);
void arcsession_gamepad_assign(arcsession *s, int idx, int port); /* -1 = auto */
int  arcsession_gamepad_assignment(arcsession *s, int idx);
int  arcsession_gamepad_effective_port(arcsession *s, int idx);
/* Bumped on every add/remove so a frontend can refresh its lists cheaply. */
unsigned arcsession_gamepad_generation(arcsession *s);
/* The last hot-plug event as text ("Connected: 8BitDo SN30 Pro (player 1)",
 * "Disconnected: ..."), for a toast; returns length, 0 if none yet. */
int  arcsession_gamepad_last_event(arcsession *s, char *dst, int dstsz);

/* ---- cross-thread system actions ------------------------------------------
 * The gamepad thread cannot call into a UI toolkit, so a system action it
 * resolves is posted here and a frontend's timer takes it. */
void arcsession_sysaction_post(arcsession *s, int sysact);
int  arcsession_sysaction_take(arcsession *s, int *out);

/* ---- the ROMs ----------------------------------------------------------------
 * Never shipped (see COMPLIANCE.md). CONFIG and every FujiNet app boot on the
 * app's own stand-in BIOS; a game needs the real one, and every machine needs
 * the video chip's character set to show text. Import copies a user's own
 * dumps into the ROM directory, each identified by size and CRC-32 against
 * MAME's tables whatever it is called, and stored under MAME's name. The
 * input can be a file, a zip (MAME's videopac.zip, i8244.zip, videopacp.zip
 * work) or a folder (searched 8 levels deep). */
typedef enum {
    ARC_ROM_BIOS = 0,          /* arcbios.rom: Arcadia 2001 and Videopac G7000 */
    ARC_ROM_BIOS_G7400,        /* g7400.bin: Videopac+ G7400 */
    ARC_ROM_FONT_8244,         /* charset_i8244.bin: the 8244/8245's characters */
    ARC_ROM_FONT_EF9340,       /* charset_ef9340_1.rom: the G7400's text chip */
    ARC_ROM_VOICE_INTERNAL,    /* sp0256b-019.bin: The Voice's SP0256 mask ROM */
    ARC_ROM_VOICE_SPEECH,      /* spr128-003.bin: The Voice's speech ROM */
    ARC_ROM_VOICE_SID,         /* spr128-004.bin: Sid the Spellbinder's speech */
    ARC_ROM_COUNT
} arc_rom_id;

typedef struct {
    const char *name;         /* "Arcadia 2001 / Videopac BIOS (arcbios.rom)" */
    const char *group;        /* "Console", "Video chips", "The Voice" */
    const char *file;         /* stored as */
    uint32_t size, crc;
} arc_rom_item;

int arcsession_rom_item_count(void);
const arc_rom_item *arcsession_rom_item(int i);
/* 1 if item i is in the ROM directory. */
int arcsession_rom_item_available(arcsession *s, int i);
/* 1 if the console BIOS for `machine` is imported / the character set(s) it
 * needs are / The Voice's ROMs are. */
int arcsession_have_bios(arcsession *s, int machine);
int arcsession_have_font(arcsession *s, int machine);
int arcsession_have_voice(arcsession *s);
/* Import from a file, a zip or a folder. Returns how many items it imported
 * (0 = recognised nothing), or -1 with the error in `msg`; `msg` always says
 * what happened (names the items, warns about an unrecognised dump of the
 * right size). Takes effect at the next power cycle. */
int arcsession_rom_import(arcsession *s, const char *path, char *msg, int msgsz);
/* 1 if `path` is a ROM this app imports (a dropped file or zip), by size and
 * CRC (a zip: by any of its entries). */
int arcsession_media_is_rom(const char *path);
const char *arcsession_roms_path(const arcsession *s);

/* ---- FujiNet -------------------------------------------------------------*/
/* Parses the "Use external FujiNet" address field: "host:port" (host a name
 * or an IPv4 literal) or a bare "port". Empty text means the default,
 * 127.0.0.1:11520. Returns 0 and fills host/port, or -1 when the text is not
 * an address:port (nothing is written then). */
int         arcsession_parse_hostport(const char *text, char *host,
                                          int hostsz, int *port);
int         arcsession_fujinet_running(const arcsession *s);
/* 1 when the session was started to dial an external FujiNet. */
int         arcsession_fujinet_external(const arcsession *s);
/* "host:port" the cartridge dials (the running session's, or the default
 * 127.0.0.1:11520 before the first start). */
const char *arcsession_boip_endpoint(const arcsession *s);
const char *arcsession_fujinet_webui_url(const arcsession *s);
int         arcsession_fujinet_copy_log(arcsession *s, char *dst, int max);
/* The cartridge's link to FujiNet: 1 up, 0 down, -1 not running. */
int         arcsession_cart_link_up(arcsession *s);
/* Status text from the cartridge ("connected", "link down", "loading 42%",
 * "game running; mailbox closed", ...). Returns length. */
int         arcsession_cart_status(arcsession *s, char *dst, int dstsz);
/* 1 once anything other than CONFIG is in the cartridge (a network boot or
 * an opened cartridge file). */
int         arcsession_cart_booted_game(arcsession *s);

/* ---- media ---------------------------------------------------------------
 * Import a cartridge into FujiNet's SD folder so CONFIG can boot it through
 * the cartridge. Returns 0 and the
 * destination path, or -1 with the error. */
int  arcsession_import_cart_to_sd(arcsession *s, const char *src_path,
                                  char *dest_out, int dest_sz);
/* Routes a dropped file: cartridge images to the cartridge directory (the
 * returned path is usable with load_cart), ROM images (BIOS, character sets,
 * speech; also zips of them) to the ROM directory (as rom_import), anything
 * else to the FujiNet SD folder. */
int  arcsession_import_media(arcsession *s, const char *src_path,
                             char *dest_out, int dest_sz);
int  arcsession_media_is_cartridge(const char *path);

const char *arcsession_config_path(const arcsession *s);
const char *arcsession_data_path(const arcsession *s);
const char *arcsession_carts_path(const arcsession *s);
const char *arcsession_sd_path(const arcsession *s);

/* ---- debugger ------------------------------------------------------------*/
arcdebug *arcsession_debugger(arcsession *s);

#ifdef __cplusplus
}
#endif

#endif /* ARCSESSION_H */
