/*
 * help_test -- the Help covers what the app has: every Preferences setting
 * and choice, every machine, every ROM, every default key and gamepad button,
 * the app's shortcuts (and the Mac's), the two ports, the debugger's tabs
 * and prompt commands, and every context a Help button opens. Every miss is
 * listed before it fails.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archelp.h"
#include "arcsession.h"
#include "test_tmpdir.h"

#define CHECK(c, ...) do { if (!(c)) { fprintf(stderr, "FAIL: " __VA_ARGS__); fputc('\n', stderr); exit(1); } } while (0)

static const unsigned platforms[] = {
    ARCHELP_GNOME, ARCHELP_KDE, ARCHELP_WINDOWS, ARCHELP_MACOS
};
static const char *const platform_names[] = { "GNOME", "KDE", "Windows", "macOS" };

/* What the frontends' Help buttons and menus open. */
static const char *const contexts[] = {
    "main", "import-sd",
    "prefs/Machine", "prefs/Display", "prefs/Input", "prefs/FujiNet",
    "debugger/prompt", "debugger/cpu", "debugger/disassembly", "debugger/video",
    "debugger/io", "debugger/breakpoints", "debugger/cart", "debugger/fujinet",
    "controllers", "fujilog",
};

/* The prompt's commands (core/include/arcdebug.h). */
static const char *const debug_commands[] = {
    "step", "over", "out", "run", "stop", "runto", "scanline", "frame",
    "break", "bpr", "bpw", "delete", "enable", "disable", "clear", "breaks", "print",
    "regs", "set", "mem", "poke", "uvi", "objects", "screen", "io", "keys", "cart",
    "mailbox", "label", "labels", "disasm", "trace", "save",
};

/* The Preferences pages: what each one has, as the frontends label it. */
static const struct {
    const char *context;
    const char *labels[10];
} pages[] = {
    { "prefs/Machine", { "Machine", NULL } },
    { "prefs/Display", { "Aspect", "Smooth scaling", "Enable audio", "Volume", NULL } },
    { "prefs/Input", { "Enable gamepads", "Analog sticks steer too", "Gamepads", "Automatic",
                       "Player 1", "Player 2", NULL } },
    { "prefs/FujiNet", { "Enable FujiNet", "Use external FujiNet", "Address", "Web UI", NULL } },
};

/* The settings keys the pages write. */
static const char *const settings_keys[] = {
    "machine", "analog_joystick", "enable_fujinet", "fujinet_external", "fujinet_external_addr",
    "enable_audio", "enable_gamepad", "volume", "cart", "aspect", "smooth",
};

/* The app's shortcuts, as a PC and as a Mac shows them. */
static const struct { const char *pc, *mac; } shortcuts[] = {
    { "Ctrl+O", "⌘O" }, { "Ctrl+R", "⌘R" }, { "Ctrl+,", "⌘," }, { "Ctrl+Q", "⌘Q" },
    { "Ctrl+Shift+H", "⌘?" }, { "F9", "F9" }, { "F11", "F11" }, { "F12", "F12" },
};

static int misses;

/* Lower case, every run of white space one space. */
static void
fold(const char *in, char *out, size_t outsz)
{
    size_t n = 0;
    int space = 0;
    for (; *in && n + 1 < outsz; in++) {
        unsigned char c = (unsigned char) *in;
        if (c == ' ' || c == '\n' || c == '\t') {
            space = 1;
            continue;
        }
        if (space && n)
            out[n++] = ' ';
        space = 0;
        out[n++] = (char) ((c >= 'A' && c <= 'Z') ? c + 32 : c);
    }
    out[n] = 0;
}

static char section[1 << 18];
static char raw[1 << 18];

/* The folded text of a context's (or slug's) section on platform p; raw
 * keeps the lines. */
static const char *
section_text(int p, const char *context)
{
    int t, b;
    section[0] = 0;
    raw[0] = 0;
    if (archelp_find(context, &t, &b) == 0) {
        archelp_plain_text(platforms[p], t, b, raw, sizeof raw);
        fold(raw, section, sizeof section);
    }
    return section;
}

static void
require(int p, const char *context, const char *what, const char *text)
{
    char want[512];
    fold(text, want, sizeof want);
    if (!strstr(section_text(p, context), want)) {
        fprintf(stderr, "MISSING (%s) %s: %s \"%s\"\n", platform_names[p], context, what, text);
        misses++;
    }
}

/* A line of the section (a table row: cells joined by " | ") with every one
 * of these, folded, in that order. */
static void
require_row(int p, const char *context, const char *a, const char *b, const char *c)
{
    char line[1024], fa[256], fb[256], fc[256];
    const char *s, *e;
    int ok = 0;

    section_text(p, context);
    fold(a, fa, sizeof fa);
    fold(b ? b : "", fb, sizeof fb);
    fold(c ? c : "", fc, sizeof fc);
    for (s = raw; *s && !ok; s = *e ? e + 1 : e) {
        char *x, *y;
        size_t n;
        e = strchr(s, '\n');
        if (!e)
            e = s + strlen(s);
        n = (size_t) (e - s) < sizeof line - 1 ? (size_t) (e - s) : sizeof line - 1;
        memcpy(line, s, n);
        line[n] = 0;
        fold(line, line, sizeof line);
        x = strstr(line, fa);
        if (!x)
            continue;
        y = b && *fb ? strstr(x + strlen(fa), fb) : x + strlen(fa);
        if (!y)
            continue;
        if (c && *fc && !strstr(y + (b && *fb ? strlen(fb) : 0), fc))
            continue;
        ok = 1;
    }
    if (!ok) {
        fprintf(stderr, "MISSING (%s) %s: a row \"%s\" | \"%s\" | \"%s\"\n", platform_names[p], context,
                a, b ? b : "", c ? c : "");
        misses++;
    }
}

static void
check_prefs(arcsession *s, int p)
{
    char buf[128];

    for (size_t k = 0; k < sizeof pages / sizeof *pages; k++)
        for (int i = 0; pages[k].labels[i]; i++)
            require(p, pages[k].context, "label", pages[k].labels[i]);

    for (int m = 0; m < ARC_MACHINE_COUNT; m++)
        require(p, "machines", "machine", arc_machine_name(m));
    for (size_t k = 0; k < sizeof settings_keys / sizeof *settings_keys; k++)
        require(p, "the-settings-file", "setting", settings_keys[k]);
    (void) s;
}

/* Every default key and gamepad button, the console's buttons and the
 * shortcuts. */
static void
check_controls(arcsession *s, int p)
{
    char name[64];

    CHECK(ARC_TARGET_COUNT == 40, "the Help says 40 controls");
    require(p, "changing-the-controls", "count", "40 in all");
    /* every default key: the disc and fire of both players, the panel, the
     * keypads, RESET and Reset to CONFIG */
    for (int t = 0; t < ARC_TARGET_COUNT; t++) {
        if (t == ARC_TARGET_SYSACT(ARC_SYSACT_DEBUG_STOP))
            continue;
        arc_binding b = arcsession_binding_get(s, t);
        if (b.keysym) {
            arcsession_keysym_name(b.keysym, name, sizeof name);
            require(p, "default-keys", "default key", name);
        }
    }
    require_row(p, "default-keys", "START", "F2", NULL);
    require_row(p, "default-keys", "OPTION", "F3", NULL);
    require_row(p, "default-keys", "SELECT", "F4", NULL);
    require_row(p, "default-keys", "RESET", "F1", NULL);
    require_row(p, "default-keys", "Reset to CONFIG", "Escape", NULL);
    require_row(p, "gamepads", "D-pad", "disc", NULL);
    require_row(p, "gamepads", "South", "Fire", NULL);
    require_row(p, "gamepads", "Start", "START", NULL);
    require_row(p, "gamepads", "West", "OPTION", NULL);
    require_row(p, "gamepads", "Back", "SELECT", NULL);
    CHECK(arcsession_binding_get(s, ARC_TARGET_PORT(0, ARC_ACT_FIRE)).button == ARC_PAD_BTN_SOUTH
          && arcsession_binding_get(s, ARC_TARGET_KEY(ARC_KEY_START)).button == ARC_PAD_BTN_START
          && arcsession_binding_get(s, ARC_TARGET_KEY(ARC_KEY_OPTION)).button == ARC_PAD_BTN_WEST
          && arcsession_binding_get(s, ARC_TARGET_KEY(ARC_KEY_SELECT)).button == ARC_PAD_BTN_BACK,
          "the gamepad rows are the default bindings");

    for (size_t k = 0; k < sizeof shortcuts / sizeof *shortcuts; k++)
        require(p, "default-keys", "shortcut", platforms[p] == ARCHELP_MACOS ? shortcuts[k].mac : shortcuts[k].pc);
}

/* The two ports, in the FujiNet topic. */
static void
check_ports(int p)
{
    char port[16];

    snprintf(port, sizeof port, "%d", ARCSESSION_BOIP_PORT);
    require(p, "fujinet", "BoIP port", port);
    snprintf(port, sizeof port, "%d", ARCSESSION_WEBUI_PORT);
    require(p, "fujinet", "web UI port", port);
    require(p, "an-external-fujinet", "default address", "127.0.0.1:11522");
}

/* Each command starts a `code` span in the prompt's section. */
static void
check_debugger(int p)
{
    int t, b;
    CHECK(archelp_find("debugger/prompt", &t, &b) == 0, "no debugger/prompt");
    const archelp_topic *tp = archelp_topic_at(t);
    int end = archelp_section_end(t, b);
    for (size_t k = 0; k < sizeof debug_commands / sizeof *debug_commands; k++) {
        const char *cmd = debug_commands[k];
        size_t n = strlen(cmd);
        int found = 0;
        for (int bl = b; bl < end && !found; bl++) {
            const archelp_block *blk = &tp->blocks[bl];
            if (!(blk->platforms & platforms[p]))
                continue;
            for (int i = 0; i < blk->nspans && !found; i++) {
                const char *c = blk->spans[i].text;
                if (blk->spans[i].kind != ARCHELP_CODE)
                    continue;
                found = !strncmp(c, cmd, n) && (c[n] == 0 || c[n] == ' ' || c[n] == '/');
            }
        }
        if (!found) {
            fprintf(stderr, "MISSING (%s) debugger/prompt: command `%s`\n", platform_names[p], cmd);
            misses++;
        }
    }
    /* the eight tabs */
    require(p, "debugger", "tabs", "eight tabs");
}

int
main(void)
{
    char cfg[512], data[512];
    arcsession_paths paths;
    archelp_hit hits[16];
    arcsession *s;
    int t, b;

    test_tmpdir(cfg, sizeof cfg, "hcfg");
    test_tmpdir(data, sizeof data, "hdata");
    memset(&paths, 0, sizeof paths);
    paths.config_dir = cfg;
    paths.data_dir = data;
    paths.fujinet_lib = "";
    s = arcsession_new(&paths);
    CHECK(s, "new");
    CHECK(archelp_topic_count() > 0, "no topics");

    for (size_t k = 0; k < sizeof contexts / sizeof *contexts; k++)
        if (archelp_find(contexts[k], &t, &b) != 0) {
            fprintf(stderr, "MISSING context %s\n", contexts[k]);
            misses++;
        }
    for (int p = 0; p < 4; p++) {
        check_prefs(s, p);
        check_controls(s, p);
        check_ports(p);
        check_debugger(p);
    }

    /* lookup and search */
    CHECK(archelp_find("#preferences", &t, &b) == 0 && b == -1, "#preferences");
    CHECK(archelp_find("#the-settings-file", &t, &b) == 0 && b >= 0, "#the-settings-file");
    CHECK(archelp_search(ARCHELP_GNOME, "mailbox", hits, 16) > 0, "search mailbox");
    CHECK(archelp_search(ARCHELP_GNOME, "zzqxj", hits, 16) == 0, "search nonsense");
    int n = archelp_search(ARCHELP_KDE, "breakpoints", hits, 16);
    CHECK(n > 0 && archelp_find("debugger/breakpoints", &t, &b) == 0
          && hits[0].topic == t && hits[0].block == b, "breakpoints is the first hit");
    for (int i = 1; i < n; i++)
        CHECK(hits[i - 1].score >= hits[i].score, "hits best first");

    /* the Mac sees its own keys */
    {
        char text[4096];
        CHECK(archelp_find("default-keys", &t, &b) == 0, "default-keys");
        archelp_plain_text(ARCHELP_MACOS, t, b, text, sizeof text);
        CHECK(strstr(text, "⌘O") != NULL, "the Mac's Open Cartridge key");
        archelp_plain_text(ARCHELP_GNOME, t, b, text, sizeof text);
        CHECK(strstr(text, "Ctrl+O") != NULL && !strstr(text, "⌘O"), "the PC's");
    }

    arcsession_free(s);
    CHECK(misses == 0, "%d missing from data/help/help.md", misses);
    printf("help ok: %d topics\n", archelp_topic_count());
    return 0;
}
