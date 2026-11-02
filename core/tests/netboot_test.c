/*
 * netboot_test -- the cartridge bring-up's own clients against the in-process
 * FujiNet, opened as cartridges (they claim the mailbox, so they are FujiNet
 * apps):
 *
 *   fujitest  one real transaction (GET_ADAPTERCONFIG_EXTENDED), its answer
 *             printed on the UVI's character screen straight from the reply
 *             window;
 *   fujiboot  MOUNT_HOST -> SET_DEVICE_FULLPATH -> MOUNT_IMAGE of
 *             /ACOUNTER.BIN on the SD host (put there by Import Cartridge to
 *             SD): FujiNet pushes it, the cart stages it, BOOTLOCK, the
 *             hand-over through the stub at $18E0, and the counter app runs.
 *
 * Run twice by ctest: with the mailbox on the cart's worker thread, and inline
 * (ARC_CART_SYNC=1 -> FUJINET_ASYNC=0). Needs ARC_TESTROM_DIR: fujinet-firmware's
 * pico/arcadia/build, built with
 *   BOOT_HOST=0 BOOT_PATH=/ACOUNTER.BIN pico/arcadia/build.sh fujiboot fujitest
 * (the pinned tree under third_party/ will do; needs Macroassembler AS).
 * SKIPs (77) without it or without the FujiNet runtime.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arcsession.h"
#include "test_arc.h"
#include "test_tmpdir.h"

static int failures;
static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failures++;
}

static int file_exists(const char *p)
{
    FILE *f = fopen(p, "rb");

    if (f)
        fclose(f);
    return f != NULL;
}

int main(void)
{
    static uint8_t img[8192];
    char cfg[512], data[512], src[700], dest[1200], fujiboot[1200], fujitest[1200], status[128];
    const char *dir = getenv("ARC_TESTROM_DIR");
    arcsession_paths p;
    arcsession *s;
    arcsession_start_opts o;

    if (!dir || !*dir) {
        printf("SKIP: ARC_TESTROM_DIR is not set (see the header)\n");
        return 77;
    }
    snprintf(fujiboot, sizeof fujiboot, "%s/fujiboot.bin", dir);
    snprintf(fujitest, sizeof fujitest, "%s/fujitest.bin", dir);
    if (!file_exists(fujiboot) || !file_exists(fujitest)) {
        printf("SKIP: no fujiboot.bin / fujitest.bin in %s\n", dir);
        return 77;
    }
    if (getenv("ARC_CART_SYNC"))
        setenv("FUJINET_ASYNC", "0", 1);

    test_tmpdir(cfg, sizeof cfg, "nbcfg");
    test_tmpdir(data, sizeof data, "nbdata");
    memset(&p, 0, sizeof p);
    p.config_dir = cfg; p.data_dir = data;
    s = arcsession_new(&p);
    if (!s) return 1;
    arcsession_default_opts(s, &o);
    o.enable_audio = 0; o.enable_gamepad = 0; o.enable_fujinet = 1;
    if (arcsession_start(s, &o) != 0) {
        printf("start failed: %s\n", arcsession_last_error(s));
        return 1;
    }
    if (!arcsession_fujinet_running(s)) {
        printf("SKIP: no FujiNet runtime available (%s)\n", arcsession_last_error(s));
        arcsession_free(s);
        return 77;
    }
    printf("mailbox %s\n", getenv("ARC_CART_SYNC") ? "inline (ARC_CART_SYNC)" : "on the cart's worker");

    snprintf(src, sizeof src, "%s/ACOUNTER.BIN", cfg);
    test_counter_image(img, 1);
    test_write_file(src, img, sizeof img);
    check(arcsession_import_cart_to_sd(s, src, dest, sizeof dest) == 0, "the counter app goes to the SD host");

    /* fujitest: one transaction, the adapter's own answer on screen */
    check(arcsession_load_cart(s, fujitest) == 0, "fujitest opens");
    {
        int got = 0;

        for (int waited = 0; waited < 20000 && !got; waited += 100) {
            got = test_find_row("127.0.0.1") >= 0 || test_find_row("ERR 00") >= 0;
            if (!got)
                test_sleep_ms(100);
        }
        if (!got)
            test_dump_screen();
        check(got, "GET_ADAPTERCONFIG_EXTENDED answered, printed from the reply window");
    }

    /* fujiboot: mount, push, stage, hand over */
    check(arcsession_load_cart(s, fujiboot) == 0, "fujiboot opens");
    {
        int swapped = 0;

        for (int waited = 0; waited < 30000 && !swapped; waited += 100) {
            swapped = arcsession_cart_booted_game(s) && test_uvi_ram(TEST_COUNTER) != 0;
            if (!swapped)
                test_sleep_ms(100);
        }
        if (!swapped)
            test_dump_screen();
        check(swapped, "the image was pushed, staged and swapped in, and is running");
    }
    {
        uint8_t a = test_uvi_ram(TEST_COUNTER);

        test_sleep_ms(300);
        check(test_uvi_ram(TEST_COUNTER) != a, "its counter moves");
    }
    arcsession_cart_status(s, status, sizeof status);
    check(strstr(status, "mailbox closed") == NULL, "and it claimed the mailbox, so the cart keeps it");

    arcsession_stop(s);
    arcsession_free(s);
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
