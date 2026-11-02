/*
 * gamepad_map_test -- the gamepad layer end to end with an SDL3 virtual
 * gamepad: hot-plug pickup, the default mapping onto the joystick (the D-pad
 * is the stick's four directions, South is its button, the left analog stick
 * steers too), a button rebound to a matrix key and to the console's RESET,
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
static int vport;   /* the stick the virtual pad was given */

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
        k = arcsession_key_held(s, 24);
        r = arcsession_switch_held(s, ARC_SW_RESET);
        if (held == want && k == key && r == reset) {
            printf("ok: %s\n", what);
            return;
        }
        sleep_ms(20);
    }
    printf("FAIL: %s (held %02X want %02X, key Q %d/%d, RESET %d/%d)\n", what, held, want, k, key, r, reset);
    failures++;
}

#define B(a) (1u << (a))

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
        printf("%s: the virtual pad got no keypad (%d; real pads attached?)\n",
               vport < 0 && arcsession_gamepad_count(s) <= baseline ? "FAIL" : "SKIP", vport);
        SDL_CloseJoystick(vj);
        SDL_DetachVirtualJoystick(vid);
        arcsession_free(s);
        return arcsession_gamepad_count(s) <= baseline ? 1 : 77;
    }
    printf("ok: the virtual pad was picked up as stick %d\n", vport + 1);

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

    /* the stick past its threshold steers too (a pad that reports its D-pad
     * as axes would otherwise drive nothing) */
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 20000);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, 20000);
    expect(s, B(ARC_ACT_RIGHT) | B(ARC_ACT_DOWN), 0, 0, "the stick right and down is right and down");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTX, 0);
    expect(s, B(ARC_ACT_RIGHT) | B(ARC_ACT_DOWN), 0, 0,
           "the D-pad holds right when the analog stick lets go of it: the sources are separate");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, 0);
    expect(s, 0, 0, 0, "centred");

    /* a button bound to a matrix key, and to the console's RESET */
    arcsession_binding_set_button(s, ARC_TARGET_KEY(24), ARC_PAD_BTN_EAST, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, true);
    expect(s, 0, 1, 0, "East, bound to Q, holds the key");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, false);
    expect(s, 0, 0, 0, "released");
    arcsession_binding_set_button(s, ARC_TARGET_SWITCH(ARC_SW_RESET), ARC_PAD_BTN_START, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_START, true);
    expect(s, 0, 0, 1, "Start, bound to RESET, holds it");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_START, false);
    expect(s, 0, 0, 0, "released");

    /* a D-pad direction rebound elsewhere stops steering */
    arcsession_binding_set_button(s, ARC_TARGET_KEY(24), ARC_PAD_BTN_DPAD_UP, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    expect(s, 0, 1, 0, "D-pad up, bound to Q, is Q and not up any more");
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    expect(s, 0, 0, 0, "released");
    arcsession_bindings_reset(s);

    /* hot-unplug while held: the stick, a key and RESET are released, not
     * left holding forever */
    arcsession_binding_set_button(s, ARC_TARGET_KEY(24), ARC_PAD_BTN_EAST, NULL, 0);
    arcsession_binding_set_button(s, ARC_TARGET_SWITCH(ARC_SW_RESET), ARC_PAD_BTN_START, NULL, 0);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_SOUTH, true);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_EAST, true);
    SDL_SetJoystickVirtualButton(vj, SDL_GAMEPAD_BUTTON_START, true);
    SDL_SetJoystickVirtualAxis(vj, SDL_GAMEPAD_AXIS_LEFTY, -20000);
    expect(s, B(ARC_ACT_RIGHT) | B(ARC_ACT_FIRE) | B(ARC_ACT_UP), 1, 1, "held before the unplug");
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
    expect(s, 0, 0, 0, "and its stick, key and RESET are released");
    arcsession_bindings_reset(s);

    arcsession_stop(s);
    arcsession_free(s);
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
