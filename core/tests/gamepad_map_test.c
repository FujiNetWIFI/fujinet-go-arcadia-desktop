/*
 * gamepad_map_test -- the gamepad layer end to end with an SDL3 virtual
 * gamepad: hot-plug pickup, the default mapping onto the disc (the D-pad is its four
 * directions, South the side button, the left analog stick the analog disc, or
 * the four directions again with analog off), a button rebound to a console
 * key (START) and to the console's RESET,
 * the sources not stepping on each other, and hot-unplug releasing everything
 * the pad held -- with no hardware and no human.
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
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
