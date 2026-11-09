/*
 * gamepad_map_test -- the gamepad layer end to end with an SDL3 virtual
 * gamepad: hot-plug pickup, the default mapping onto the disc (the D-pad is its four
 * directions, South the side button, the left analog stick the analog disc, or
 * the four directions again with analog off), a button rebound to a console
 * key (START) and to the console's RESET,
 * the sources not stepping on each other, and hot-unplug releasing everything
 * the pad held -- with no hardware and no human. Then the pad's port
 * assignment: written to the settings ("pad_assign") when made, restored when
 * a new session (a restart) finds the pad again, found through a stale GUID
 * by the name, kept through garbage, and removed by Automatic.
 *
 * What the pad holds is read back through arcsession_buttons_held,
 * arcsession_key_held and arcsession_switch_held, which report the same
 * per-source bits the machine is driven from. SKIPs (77) where SDL has no
 * virtual joysticks or a machine's real pads leave no port free.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arcsession.h"
#include "test_tmpdir.h"

#define PAD_NAME "arctest virtual pad"

static int failures;
static int vport;   /* the disc port the virtual pad was given */

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok)
        failures++;
}

static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* The pad thread syncs devices a few times a second and polls state
 * faster; give it two seconds. */
static void expect(arcsession *s, unsigned want, int key, int reset, const char *what)
{
    unsigned held = 0;
    int k = 0, r = 0;
    for (int i = 0; i < 100; i++) {
        held = arcsession_buttons_held(s, vport);
        k = arcsession_key_held(s, ARC_KEY_START);
        r = arcsession_switch_held(s, ARC_SW_RESET);
        if (held == want && k == key && r == reset) {
            printf("ok: %s\n", what);
            return;
        }
        sleep_ms(20);
    }
    printf("FAIL: %s (held %02X want %02X, key START %d/%d, RESET %d/%d)\n", what, held, want, k, key, r, reset);
    failures++;
}

#define B(a) (1u << (a))

/* The analog disc, by sign only (-1, 0, +1 per axis): the exact numbers are
 * the dead zone's business, tested in gamepad_test. */
static void expect_disc(arcsession *s, int sx, int sy, const char *what)
{
    int x = 0, y = 0;
    for (int i = 0; i < 100; i++) {
        arcsession_disc_get(s, vport, &x, &y);
        if (((x > 0) - (x < 0)) == sx && ((y > 0) - (y < 0)) == sy) {
            printf("ok: %s\n", what);
            return;
        }
        sleep_ms(20);
    }
    printf("FAIL: %s (disc %d,%d want signs %d,%d)\n", what, x, y, sx, sy);
    failures++;
}

/* A session on p's directories with only the gamepad thread. */
static arcsession *session_up(arcsession_paths *p)
{
    arcsession *s = arcsession_new(p);
    arcsession_start_opts o;
    if (!s)
        return NULL;
    arcsession_default_opts(s, &o);
    o.enable_fujinet = 0;
    o.enable_audio = 0;
    o.enable_gamepad = 1;
    if (arcsession_start(s, &o) != 0) {
        printf("FAIL: start: %s\n", arcsession_last_error(s));
        arcsession_free(s);
        return NULL;
    }
    return s;
}

/* The virtual pad's slot now, or -1. */
static int pad_slot_now(arcsession *s)
{
    for (int idx = 0; idx < arcsession_gamepad_count(s); idx++) {
        char name[64];
        arcsession_gamepad_name(s, idx, name, sizeof name);
        if (strcmp(name, PAD_NAME) == 0)
            return idx;
    }
    return -1;
}

/* The virtual pad's slot once the pad thread has found it (polled, up to
 * five seconds: a slow runner is slow to enumerate), or -1. */
static int wait_for_pad(arcsession *s)
{
    int idx = pad_slot_now(s);
    for (int tries = 0; tries < 250 && idx < 0; tries++) {
        sleep_ms(20);
        idx = pad_slot_now(s);
    }
    return idx;
}

/* The pad_assign setting with the pad's entry in it? */
static int has_entry(arcsession *s, const char *guid, int port)
{
    char want[160];
    snprintf(want, sizeof want, "%s|%s=%d", guid, PAD_NAME, port);
    return strstr(arcsession_get_str(s, "pad_assign", ""), want) != NULL;
}

/* A stale entry (GUID guid, the pad's name) goes to a different pad of the
 * same name that connects first, by the name fallback. The pad with exactly
 * that GUID then connects and takes it over: the decoy goes automatic. No pad
 * is attached on entry; both are detached on return. */
static void takeover(arcsession_paths *p, SDL_VirtualJoystickDesc *desc,
                     const char *guid, int want)
{
    SDL_VirtualJoystickDesc other = *desc;
    SDL_JoystickID decoy, owner = 0;
    char text[300], gid[64];
    arcsession *s = session_up(p);
    int idx;

    if (!s) { failures++; return; }
    snprintf(text, sizeof text, "%s|%s=%d", guid, PAD_NAME, want);
    arcsession_set_str(s, "pad_assign", text);
    arcsession_free(s);

    other.vendor_id = (Uint16)(desc->vendor_id ^ 0x5a5a);
    other.product_id = (Uint16)(desc->product_id ^ 0xa5a5);
    decoy = SDL_AttachVirtualJoystick(&other);
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(decoy), gid, sizeof gid);
    if (!decoy || !strcmp(gid, guid)) {
        printf("skip: the takeover test needs a second virtual pad with another GUID\n");
        if (decoy)
            SDL_DetachVirtualJoystick(decoy);
        return;
    }
    s = session_up(p);
    if (!s) { failures++; SDL_DetachVirtualJoystick(decoy); return; }
    idx = wait_for_pad(s);
    check(idx >= 0 && arcsession_gamepad_assignment(s, idx) == want,
          "takeover: a same-name pad with another GUID gets the entry by name");
    owner = SDL_AttachVirtualJoystick(desc);
    for (int tries = 0; tries < 250 && arcsession_gamepad_count(s) < 2; tries++)
        sleep_ms(20);
    check(owner && arcsession_gamepad_count(s) == 2, "takeover: the second pad is picked up");
    if (arcsession_gamepad_count(s) == 2) {
        check(arcsession_gamepad_assignment(s, 1) == want,
              "the pad with the entry's GUID takes the assignment over");
        check(arcsession_gamepad_assignment(s, 0) == -1, "and the name-only pad is automatic");
    }
    arcsession_free(s);
    if (owner)
        SDL_DetachVirtualJoystick(owner);
    SDL_DetachVirtualJoystick(decoy);
}

/* A pad's port assignment survives a restart. The virtual pad stays attached
 * across the sessions (SDL is held open by the caller). */
static void persistence(arcsession_paths *p)
{
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID vid;
    arcsession *s;
    char guid[64];
    int idx, want, auto_port;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = PAD_NAME;
    vid = SDL_AttachVirtualJoystick(&desc);
    if (!vid) {
        printf("FAIL: the virtual pad would not attach again: %s\n", SDL_GetError());
        failures++;
        return;
    }
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(vid), guid, sizeof guid);

    s = session_up(p);
    if (!s) { failures++; SDL_DetachVirtualJoystick(vid); return; }
    idx = wait_for_pad(s);
    check(idx >= 0, "persistence: the pad is picked up");
    if (idx < 0) { arcsession_free(s); SDL_DetachVirtualJoystick(vid); return; }
    check(arcsession_gamepad_assignment(s, idx) == -1, "a pad nobody assigned is automatic");
    check(strstr(arcsession_get_str(s, "pad_assign", ""), guid) == NULL, "and has no entry");
    auto_port = arcsession_gamepad_effective_port(s, idx);
    want = auto_port == 0 ? 1 : 0;

    arcsession_gamepad_assign(s, idx, want);
    check(arcsession_gamepad_assignment(s, idx) == want
          && arcsession_gamepad_effective_port(s, idx) == want, "assigning drives the other port");
    check(has_entry(s, guid, want), "and writes <guid>|<name>=<port> to pad_assign");

    /* the file itself, not just the in-memory store */
    {
        char path[600], line[2048];
        int found = 0;
        FILE *fp;
        snprintf(path, sizeof path, "%s/settings.ini", p->config_dir);
        fp = fopen(path, "r");
        if (fp) {
            while (fgets(line, sizeof line, fp))
                if (!strncmp(line, "pad_assign=", 11) && strstr(line, guid))
                    found = 1;
            fclose(fp);
        }
        check(found, "the settings file on disk holds it");
    }

    /* restart: a new session on the same settings */
    arcsession_free(s);
    s = session_up(p);
    if (!s) { failures++; SDL_DetachVirtualJoystick(vid); return; }
    idx = wait_for_pad(s);
    check(idx >= 0, "restart: the pad is picked up again");
    if (idx >= 0) {
        check(arcsession_gamepad_assignment(s, idx) == want, "the assignment is restored on device add");
        check(arcsession_gamepad_effective_port(s, idx) == want, "and the pad drives that port");
    }

    /* a stale GUID (another USB port): the name finds the entry. Rewrite the
     * stored GUID, restart. */
    {
        char text[300];
        snprintf(text, sizeof text, "ffffffffffffffffffffffffffffffff|%s=%d", PAD_NAME, want);
        arcsession_set_str(s, "pad_assign", text);
    }
    arcsession_free(s);
    s = session_up(p);
    if (!s) { failures++; SDL_DetachVirtualJoystick(vid); return; }
    idx = wait_for_pad(s);
    check(idx >= 0 && arcsession_gamepad_assignment(s, idx) == want,
          "a different GUID with the same name gets the assignment too");

    /* Automatic removes the entry, and the next run starts automatic */
    arcsession_gamepad_assign(s, idx, want);   /* back under its own GUID */
    check(has_entry(s, guid, want), "assigned under the pad's own GUID again");
    arcsession_gamepad_assign(s, idx, -1);
    check(arcsession_gamepad_assignment(s, idx) == -1, "Automatic: no explicit port");
    check(strstr(arcsession_get_str(s, "pad_assign", ""), PAD_NAME) == NULL,
          "and the entry is gone from pad_assign");
    arcsession_free(s);
    s = session_up(p);
    if (!s) { failures++; SDL_DetachVirtualJoystick(vid); return; }
    idx = wait_for_pad(s);
    check(idx >= 0 && arcsession_gamepad_assignment(s, idx) == -1
          && arcsession_gamepad_effective_port(s, idx) == auto_port, "after a restart it is automatic again");

    /* garbage in the setting is ignored, silently */
    arcsession_set_str(s, "pad_assign", ";;|=;zz|x=9;%%%;" "00000000000000000000000000000000|" PAD_NAME "=7");
    arcsession_free(s);
    s = session_up(p);
    if (!s) { failures++; SDL_DetachVirtualJoystick(vid); return; }
    idx = wait_for_pad(s);
    check(idx >= 0 && arcsession_gamepad_assignment(s, idx) == -1,
          "a malformed or out-of-range pad_assign leaves the pad automatic");
    arcsession_gamepad_assign(s, idx, want);
    check(has_entry(s, guid, want), "and a new assignment still writes");
    arcsession_gamepad_assign(s, idx, -1);

    /* a hot replug within a run restores it too */
    arcsession_gamepad_assign(s, idx, want);
    SDL_DetachVirtualJoystick(vid);
    for (int tries = 0; tries < 250 && pad_slot_now(s) >= 0; tries++)
        sleep_ms(20);
    check(pad_slot_now(s) < 0, "unplugged: the pad is gone");
    vid = SDL_AttachVirtualJoystick(&desc);
    idx = vid ? wait_for_pad(s) : -1;
    check(idx >= 0 && arcsession_gamepad_assignment(s, idx) == want, "unplugged and plugged back in: the port comes back");
    arcsession_gamepad_assign(s, idx, -1);

    arcsession_free(s);
    if (vid)
        SDL_DetachVirtualJoystick(vid);
    takeover(p, &desc, guid, want);
}

int main(void)
{
    char cfg[512], data[512];
    arcsession_paths p;
    arcsession *s;
    arcsession_start_opts o;
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID vid;
    SDL_Joystick *vj;
    int i, baseline;

    /* held open for the whole run: a session stopping must not take the
     * virtual pads with it (the persistence tests restart sessions) */
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
        return 1;
    test_tmpdir(cfg, sizeof cfg, "gmcfg");
    test_tmpdir(data, sizeof data, "gmdata");
    memset(&p, 0, sizeof p);
    p.config_dir = cfg;
    p.data_dir = data;
    p.fujinet_lib = "";      /* the pad only */
    s = arcsession_new(&p);
    if (!s)
        return 1;
    arcsession_default_opts(s, &o);
    o.enable_fujinet = 0;
    o.enable_audio = 0;
    o.enable_gamepad = 1;
    if (arcsession_start(s, &o) != 0) {
        printf("FAIL: start: %s\n", arcsession_last_error(s));
        return 1;
    }

    /* Real pads may already be attached: let the first device scan find
     * them, so the virtual one is known by the slot it adds. */
    sleep_ms(700);
    baseline = arcsession_gamepad_count(s);

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = PAD_NAME;
    vid = SDL_AttachVirtualJoystick(&desc);
    if (!vid) {
        printf("SKIP: no virtual joysticks here: %s\n", SDL_GetError());
        arcsession_free(s);
        return 77;
    }
    vj = SDL_OpenJoystick(vid);
    if (!vj)
        return 1;

    for (i = 0; i < 100 && arcsession_gamepad_count(s) <= baseline; i++)
        sleep_ms(20);
    vport = -1;
    for (i = 0; i < arcsession_gamepad_count(s); i++) {
        char name[64];
        arcsession_gamepad_name(s, i, name, sizeof name);
        if (strcmp(name, PAD_NAME) == 0)
            vport = arcsession_gamepad_effective_port(s, i);
    }
    if (vport < 0 || vport > 1) {
        printf("%s: the virtual pad got no port (%d; real pads attached?)\n",
               vport < 0 && arcsession_gamepad_count(s) <= baseline ? "FAIL" : "SKIP", vport);
        SDL_CloseJoystick(vj);
        SDL_DetachVirtualJoystick(vid);
        arcsession_free(s);
        return arcsession_gamepad_count(s) <= baseline ? 1 : 77;
    }
    printf("ok: the virtual pad was picked up as disc %d\n", vport + 1);

    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_SOUTH, true);
    expect(s, B(ARC_ACT_FIRE), 0, 0, "South is the button");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_SOUTH, false);
    expect(s, 0, 0, 0, "released");

    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, true);
    expect(s, 0, 0, 0, "East is nothing by default");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, false);

    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    expect(s, B(ARC_ACT_UP), 0, 0, "D-pad up is up");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    expect(s, B(ARC_ACT_UP) | B(ARC_ACT_LEFT), 0, 0, "up and left together are a diagonal");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
    expect(s, B(ARC_ACT_LEFT), 0, 0, "up and down together cancel");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    expect(s, B(ARC_ACT_LEFT) | B(ARC_ACT_DOWN), 0, 0, "and down is back once up lets go");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
    expect(s, 0, 0, 0, "released");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    expect(s, B(ARC_ACT_RIGHT), 0, 0, "D-pad right is right");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
    expect(s, 0, 0, 0, "released");

    /* the left stick is the analog disc by default: read back through
     * arcsession_disc_get, and the digital directions stay released */
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 20000);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, -32768);
    expect_disc(s, 1, -1, "the stick right and up is a disc deflected right and up");
    expect(s, 0, 0, 0, "and holds no digital direction");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    expect(s, B(ARC_ACT_LEFT), 0, 0, "the D-pad is still its own source");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 0);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, 0);
    expect_disc(s, 0, 0, "the stick centred: the disc is centred");
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 2000);
    expect_disc(s, 0, 0, "a tremor inside the dead zone is nothing");
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 0);

    /* with analog off the stick is the old eight-way switch */
    arcsession_set_analog(s, 0);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 20000);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, 20000);
    expect(s, B(ARC_ACT_RIGHT) | B(ARC_ACT_DOWN), 0, 0, "analog off: the stick right and down is right and down");
    expect_disc(s, 0, 0, "and the disc stays centred");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 0);
    expect(s, B(ARC_ACT_RIGHT) | B(ARC_ACT_DOWN), 0, 0,
           "the D-pad holds right when the analog stick lets go of it: the sources are separate");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, 0);
    expect(s, 0, 0, 0, "centred");
    arcsession_set_analog(s, 1);

    /* a button bound to a console key, and to the console's RESET */
    arcsession_binding_set_button(s, ARC_TARGET_KEY(ARC_KEY_START), ARC_PAD_BTN_EAST, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, true);
    expect(s, 0, 1, 0, "East, bound to START, holds the key");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, false);
    expect(s, 0, 0, 0, "released");
    arcsession_binding_set_button(s, ARC_TARGET_SWITCH(ARC_SW_RESET), ARC_PAD_BTN_START, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_START, true);
    expect(s, 0, 0, 1, "Start, bound to RESET, holds it");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_START, false);
    expect(s, 0, 0, 0, "released");

    /* a D-pad direction rebound elsewhere stops steering */
    arcsession_binding_set_button(s, ARC_TARGET_KEY(ARC_KEY_START), ARC_PAD_BTN_DPAD_UP, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    expect(s, 0, 1, 0, "D-pad up, bound to START, is START and not up any more");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    expect(s, 0, 0, 0, "released");
    arcsession_bindings_reset(s);

    /* hot-unplug while held: the stick, a key and RESET are released, not
     * left holding forever */
    arcsession_binding_set_button(s, ARC_TARGET_KEY(ARC_KEY_START), ARC_PAD_BTN_EAST, NULL, 0);
    arcsession_binding_set_button(s, ARC_TARGET_SWITCH(ARC_SW_RESET), ARC_PAD_BTN_START, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_SOUTH, true);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, true);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_START, true);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 30000);
    expect(s, B(ARC_ACT_RIGHT) | B(ARC_ACT_FIRE), 1, 1, "held before the unplug");
    expect_disc(s, 1, 0, "with the disc deflected");
    SDL_CloseJoystick(vj);
    SDL_DetachVirtualJoystick(vid);
    for (i = 0; i < 100 && arcsession_gamepad_count(s) != baseline; i++)
        sleep_ms(20);
    if (arcsession_gamepad_count(s) == baseline)
        printf("ok: the unplugged pad is gone\n");
    else {
        printf("FAIL: the backend kept the detached pad (%d, baseline %d)\n",
               arcsession_gamepad_count(s), baseline);
        failures++;
    }
    expect(s, 0, 0, 0, "and its buttons, key and RESET are released");
    expect_disc(s, 0, 0, "and its disc is centred");
    arcsession_bindings_reset(s);

    arcsession_stop(s);
    arcsession_free(s);

    persistence(&p);
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
