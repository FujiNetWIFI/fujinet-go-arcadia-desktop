/*
 * bindings_test -- the remappable table and the keyboard translator, pure:
 * the defaults (two keypads, two discs, F1 for RESET), that no two
 * targets share a default, steal-and-describe, persistence through the
 * settings store, and names. No machine is started.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>

#include "arcsession.h"
#include "test_tmpdir.h"

static int failures;
static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
}

static int key_target(arcsession *s, uint32_t keysym)
{
    return arcsession_target_for_key(s, keysym);
}

int main(void)
{
    char cfg[512], data[512], stolen[96], name[64];
    arcsession_paths p;
    arcsession *s;
    arc_binding b;
    int t, u, clash = 0, tkey;

    test_tmpdir(cfg, sizeof cfg, "bcfg");
    test_tmpdir(data, sizeof data, "bdata");
    memset(&p, 0, sizeof p);
    p.config_dir = cfg; p.data_dir = data; p.fujinet_lib = "";
    s = arcsession_new(&p);
    if (!s) return 1;

    /* the table's shape */
    check(ARC_TARGET_COUNT == 10 + 27 + 1 + 2, "ten disc targets, 27 console keys, RESET, two actions");
    check(ARC_TARGET_KEY(0) == 10 && ARC_TARGET_SWITCH(ARC_SW_RESET) == 37
          && ARC_TARGET_SYSACT(0) == 38, "the flat target indices");

    /* defaults: player 1's keypad on the digits */
    check(key_target(s, '1') == ARC_TARGET_KEY(0) && key_target(s, '5') == ARC_TARGET_KEY(4)
          && key_target(s, '9') == ARC_TARGET_KEY(8) && key_target(s, '0') == ARC_TARGET_KEY(10),
          "player 1's keypad is the top-row digits (1-9, then 0)");
    check(key_target(s, ARC_KEYSYM_BACKSPACE) == ARC_TARGET_KEY(9), "Backspace is player 1's CLEAR");
    check(key_target(s, ARC_KEYSYM_RETURN) == ARC_TARGET_KEY(11), "Enter is player 1's ENTER");
    check(key_target(s, ARC_KEYSYM_KP_ENTER) == ARC_TARGET_KEY(11), "so is the keypad's Enter");
    /* player 2's keypad is the Q block */
    check(key_target(s, 'q') == ARC_TARGET_KEY(12) && key_target(s, 'w') == ARC_TARGET_KEY(13)
          && key_target(s, 'e') == ARC_TARGET_KEY(14) && key_target(s, 'a') == ARC_TARGET_KEY(15)
          && key_target(s, 's') == ARC_TARGET_KEY(16) && key_target(s, 'd') == ARC_TARGET_KEY(17)
          && key_target(s, 'z') == ARC_TARGET_KEY(18) && key_target(s, 'x') == ARC_TARGET_KEY(19)
          && key_target(s, 'c') == ARC_TARGET_KEY(20) && key_target(s, 'v') == ARC_TARGET_KEY(21)
          && key_target(s, 'b') == ARC_TARGET_KEY(22) && key_target(s, 'n') == ARC_TARGET_KEY(23),
          "player 2's keypad is Q W E / A S D / Z X C / V B N");
    check(key_target(s, 'Q') == ARC_TARGET_KEY(12), "lookup folds case");
    check(key_target(s, ARC_KEYSYM_F2) == ARC_TARGET_KEY(ARC_KEY_START)
          && key_target(s, ARC_KEYSYM_F3) == ARC_TARGET_KEY(ARC_KEY_OPTION)
          && key_target(s, ARC_KEYSYM_F4) == ARC_TARGET_KEY(ARC_KEY_SELECT),
          "START, OPTION and SELECT are F2, F3 and F4");

    /* the discs */
    check(key_target(s, ARC_KEYSYM_UP) == ARC_TARGET_PORT(0, ARC_ACT_UP)
          && key_target(s, ARC_KEYSYM_RIGHT) == ARC_TARGET_PORT(0, ARC_ACT_RIGHT)
          && key_target(s, ARC_KEYSYM_DOWN) == ARC_TARGET_PORT(0, ARC_ACT_DOWN)
          && key_target(s, ARC_KEYSYM_LEFT) == ARC_TARGET_PORT(0, ARC_ACT_LEFT)
          && key_target(s, ARC_KEYSYM_RCTRL) == ARC_TARGET_PORT(0, ARC_ACT_FIRE),
          "disc 1 is the arrow keys and Right Ctrl");
    check(key_target(s, ARC_KEYSYM_KP_8) == ARC_TARGET_PORT(1, ARC_ACT_UP)
          && key_target(s, ARC_KEYSYM_KP_6) == ARC_TARGET_PORT(1, ARC_ACT_RIGHT)
          && key_target(s, ARC_KEYSYM_KP_2) == ARC_TARGET_PORT(1, ARC_ACT_DOWN)
          && key_target(s, ARC_KEYSYM_KP_4) == ARC_TARGET_PORT(1, ARC_ACT_LEFT)
          && key_target(s, ARC_KEYSYM_KP_0) == ARC_TARGET_PORT(1, ARC_ACT_FIRE),
          "disc 2 is the keypad's 8 6 2 4 and 0");
    check(key_target(s, ARC_KEYSYM_KP_5) == -1 && key_target(s, ARC_KEYSYM_KP_7) == -1,
          "the keypad's other digits are left alone");
    b = arcsession_binding_get(s, ARC_TARGET_SWITCH(ARC_SW_RESET));
    check(b.keysym == ARC_KEYSYM_F1 && b.button == ARC_PAD_BTN_NONE, "RESET defaults to F1");
    b = arcsession_binding_get(s, ARC_TARGET_SYSACT(ARC_SYSACT_RESET_CONFIG));
    check(b.keysym == ARC_KEYSYM_ESCAPE, "Reset to CONFIG defaults to Escape");
    check(key_target(s, ARC_KEYSYM_F9) == -1 && key_target(s, ARC_KEYSYM_F11) == -1
          && key_target(s, ARC_KEYSYM_F12) == -1, "F9, F11 and F12 are the frontends'");

    /* every default is its own: no key drives two targets */
    for (t = 0; t < ARC_TARGET_COUNT; t++) {
        arc_binding bt = arcsession_binding_get(s, t);
        if (!bt.keysym) continue;
        for (u = t + 1; u < ARC_TARGET_COUNT; u++)
            if (arcsession_binding_get(s, u).keysym == bt.keysym) {
                printf("  (%s and %s share a key)\n", arc_target_name(t), arc_target_name(u));
                clash++;
            }
    }
    check(clash == 0, "no two default keys clash");

    /* gamepad defaults, the same on both discs */
    for (t = 0; t < 2; t++) {
        check(arcsession_binding_get(s, ARC_TARGET_PORT(t, ARC_ACT_UP)).button == ARC_PAD_BTN_DPAD_UP
              && arcsession_binding_get(s, ARC_TARGET_PORT(t, ARC_ACT_RIGHT)).button == ARC_PAD_BTN_DPAD_RIGHT
              && arcsession_binding_get(s, ARC_TARGET_PORT(t, ARC_ACT_DOWN)).button == ARC_PAD_BTN_DPAD_DOWN
              && arcsession_binding_get(s, ARC_TARGET_PORT(t, ARC_ACT_LEFT)).button == ARC_PAD_BTN_DPAD_LEFT
              && arcsession_binding_get(s, ARC_TARGET_PORT(t, ARC_ACT_FIRE)).button == ARC_PAD_BTN_SOUTH,
              t ? "the second pad: D-pad and South" : "the first pad: D-pad and South");
        check(arcsession_target_for_button(s, t, ARC_PAD_BTN_SOUTH) == ARC_TARGET_PORT(t, ARC_ACT_FIRE),
              "button lookup is scoped to the disc");
        check(arcsession_target_for_button(s, t, ARC_PAD_BTN_START) == ARC_TARGET_KEY(ARC_KEY_START)
              && arcsession_target_for_button(s, t, ARC_PAD_BTN_WEST) == ARC_TARGET_KEY(ARC_KEY_OPTION)
              && arcsession_target_for_button(s, t, ARC_PAD_BTN_BACK) == ARC_TARGET_KEY(ARC_KEY_SELECT),
              "Start, X and Back are START, OPTION and SELECT on either pad");
    }
    check(arcsession_target_for_button(s, 0, ARC_PAD_BTN_NORTH) == -1, "North is nothing");

    /* steal: one key, one target */
    arcsession_binding_set_key(s, ARC_TARGET_KEY(13), ARC_KEYSYM_LEFT, stolen, sizeof stolen);
    check(strstr(stolen, "Disc 1: Left") != NULL, "rebinding Left reports the holder it displaced");
    check(arcsession_binding_get(s, ARC_TARGET_PORT(0, ARC_ACT_LEFT)).keysym == 0, "the old holder lost the key");
    check(key_target(s, ARC_KEYSYM_LEFT) == ARC_TARGET_KEY(13), "the new holder has it");
    arcsession_binding_set_key(s, ARC_TARGET_KEY(13), 'w', stolen, sizeof stolen);
    check(stolen[0] == '\0' && key_target(s, 'w') == ARC_TARGET_KEY(13),
          "putting W back takes nothing from anyone");
    arcsession_binding_set_key(s, ARC_TARGET_PORT(0, ARC_ACT_LEFT), ARC_KEYSYM_LEFT, NULL, 0);

    arcsession_binding_set_button(s, ARC_TARGET_KEY(15), ARC_PAD_BTN_WEST, NULL, 0);
    check(arcsession_target_for_button(s, 0, ARC_PAD_BTN_WEST) == ARC_TARGET_KEY(15)
          && arcsession_target_for_button(s, 1, ARC_PAD_BTN_WEST) == ARC_TARGET_KEY(15),
          "a pad button bound to a console key works on either pad");
    check(arcsession_binding_get(s, ARC_TARGET_KEY(ARC_KEY_OPTION)).button == ARC_PAD_BTN_NONE,
          "and took it from OPTION");
    arcsession_binding_set_button(s, ARC_TARGET_PORT(1, ARC_ACT_FIRE), ARC_PAD_BTN_WEST, stolen, sizeof stolen);
    check(strstr(stolen, "Key: P2 keypad 4") != NULL, "stealing a pad button names the key that had it");
    check(arcsession_binding_get(s, ARC_TARGET_KEY(15)).button == ARC_PAD_BTN_NONE, "which lost it");
    arcsession_binding_set_key(s, ARC_TARGET_KEY(15), 0, NULL, 0);
    check(arcsession_binding_get(s, ARC_TARGET_KEY(15)).keysym == 0, "a key can be unbound");

    /* names */
    arcsession_keysym_name(ARC_KEYSYM_F12, name, sizeof name);
    check(strcmp(name, "F12") == 0, "F12 names itself");
    arcsession_keysym_name('a', name, sizeof name);
    check(strcmp(name, "A") == 0, "letters name upper-case");
    arcsession_keysym_name(ARC_KEYSYM_KP_7, name, sizeof name);
    check(strcmp(name, "Keypad 7") == 0, "numeric keypad keys say so");
    arcsession_keysym_name(0xffab, name, sizeof name);
    check(strcmp(name, "Keypad +") == 0, "the keypad's + too");
    check(strcmp(arc_pad_button_name(ARC_PAD_BTN_DPAD_LEFT), "D-pad Left") == 0, "pad button names");
    check(strcmp(arc_target_name(ARC_TARGET_SYSACT(ARC_SYSACT_RESET_CONFIG)), "Reset to CONFIG") == 0,
          "system action names");
    check(strcmp(arc_target_name(ARC_TARGET_PORT(1, ARC_ACT_FIRE)), "Disc 2: Fire") == 0, "disc target names");
    check(strcmp(arc_target_name(ARC_TARGET_PORT(0, ARC_ACT_UP)), "Disc 1: Up") == 0, "and the first disc's");
    check(strcmp(arc_target_name(ARC_TARGET_KEY(4)), "Key: P1 keypad 5") == 0, "keypad key names");
    check(strcmp(arc_target_name(ARC_TARGET_KEY(21)), "Key: P2 keypad CLEAR") == 0, "player 2's CLEAR");
    check(strcmp(arc_target_name(ARC_TARGET_KEY(ARC_KEY_START)), "Key: START") == 0, "the panel's buttons");
    check(strcmp(arc_target_short_name(ARC_TARGET_PORT(0, ARC_ACT_RIGHT)), "Right") == 0, "short names");
    check(strcmp(arc_target_short_name(ARC_TARGET_KEY(11)), "P1 keypad ENTER") == 0, "short key names");
    check(strcmp(arc_target_short_name(ARC_TARGET_KEY(ARC_KEY_SELECT)), "SELECT") == 0, "short panel names");
    check(strcmp(arc_target_short_name(ARC_TARGET_SWITCH(ARC_SW_RESET)), "RESET") == 0, "the console's button");
    tkey = 0;
    for (t = 0; t < ARC_TARGET_COUNT; t++)
        if (arc_target_name(t)[0] == '\0') tkey++;
    check(tkey == 0, "every target has a name");

    /* native key codes: evdev KEY_1 is 2, Windows scancode 0x02 is '1', macOS 0x12 is '1' */
    check(arcsession_keysym_from_evdev(2) == '1', "evdev KEY_1 -> '1'");
    check(arcsession_keysym_from_win_scancode(0x02, 0) == '1', "Windows scancode 02 -> '1'");
    check(arcsession_keysym_from_macos_keycode(0x12) == '1', "macOS keycode 0x12 -> '1'");
    check(arcsession_keysym_from_evdev(103) == ARC_KEYSYM_UP, "evdev KEY_UP -> Up");
    check(arcsession_keysym_from_win_scancode(0x48, 1) == ARC_KEYSYM_UP, "Windows E0 48 -> Up");
    check(arcsession_keysym_from_macos_keycode(0x7E) == ARC_KEYSYM_UP, "macOS 0x7E -> Up");
    check(arcsession_keysym_from_evdev(53) == '/', "evdev KEY_SLASH -> '/' (the ? key)");
    check(arcsession_keysym_from_evdev(78) == 0xffab, "evdev KEY_KPPLUS -> the keypad's +");

    /* persistence */
    arcsession_binding_set_key(s, ARC_TARGET_PORT(1, ARC_ACT_UP), 'w', stolen, sizeof stolen);
    check(strstr(stolen, "Key: P2 keypad 2") != NULL, "rebinding W names the key it took it from");
    arcsession_free(s);
    s = arcsession_new(&p);
    check(s && key_target(s, 'w') == ARC_TARGET_PORT(1, ARC_ACT_UP), "the rebinding persisted");
    check(s && key_target(s, ARC_KEYSYM_KP_8) == -1, "and what it displaced stayed displaced");
    arcsession_bindings_reset(s);
    check(key_target(s, 'w') == ARC_TARGET_KEY(13), "reset restores the defaults");
    check(key_target(s, ARC_KEYSYM_KP_8) == ARC_TARGET_PORT(1, ARC_ACT_UP), "all of them");
    check(strcmp(arcsession_get_str(s, "bindings", "x"), "") == 0, "and persists nothing");
    arcsession_free(s);

    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
