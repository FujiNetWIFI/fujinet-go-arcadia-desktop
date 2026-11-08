/*
 * window.c -- the main window: the display, a header-bar menu, keyboard
 * capture, the console's RESET key, cartridge opening and drops, gamepad
 * hot-plug toasts and the FujiNet status.
 *
 * Keyboard events are translated by hardware keycode (evdev, via the
 * session's HID table) rather than by GDK keyval, so a binding names the
 * physical key whatever Shift is doing and whatever layout is active -- the
 * same path the Qt, Win32 and AppKit frontends take, which is what lets one
 * tested table serve all four.
 *
 * The console's own RESET key is a machine key, not a menu accelerator: F1 is
 * RESET through the bindings (MAME's key), so it can be remapped and a
 * gamepad's button drives it too. The menu items name whatever key is bound
 * to them now.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "window.h"

#include "controllers/controllers_window.h"
#include "debugger/dbg_window.h"
#include "display.h"
#include "fujilog.h"
#include "help/help_window.h"
#include "prefs.h"

#include <string.h>

struct _ArcWindow {
    AdwApplicationWindow parent_instance;

    arcsession *session;
    GtkWidget *display;
    GtkWidget *toast_overlay;
    GtkWidget *status;          /* the FujiNet link indicator */
    GtkWidget *status_dot;
    GMenu *console_menu;        /* the Console section, relabelled live */
    char console_labels[256];   /* what it last showed */
    guint status_id;
    guint sysact_id;
    gboolean sysact_down[ARC_SYSACT_COUNT];
    unsigned pad_generation;
    gboolean fullscreen;
};

G_DEFINE_FINAL_TYPE(ArcWindow, arc_window, ADW_TYPE_APPLICATION_WINDOW)

void arc_window_toast(ArcWindow *self, const char *text)
{
    adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(self->toast_overlay),
                                adw_toast_new(text));
}

/* ---- the accent ------------------------------------------------------------ */

static GtkCssProvider *s_accent_css;

/* The accent moved toward white by t, for text on a dark theme: the
 * keypad gold is too dark to read as a thin line there. */
static guint32 tint(guint32 rgb, double t)
{
    guint32 out = 0;
    int s;
    for (s = 16; s >= 0; s -= 8) {
        double c = (rgb >> s) & 0xFF;
        out |= (guint32)(c + (255 - c) * t + 0.5) << s;
    }
    return out;
}

static void load_accent_css(gboolean dark)
{
    char buf[1024];
    /* Dark text sits on the gold everywhere; as a text colour the gold is
     * only readable lightened on a dark theme, and the darker bronze
     * stands in on a light one. */
    g_snprintf(buf, sizeof buf,
        ".arc-accent { background: #%06x; color: #1e1e1e; }\n"
        ".arc-accent:hover { background: shade(#%06x, 1.12); }\n"
        ".arc-accent-text { color: #%06x; font-weight: bold; }\n"
        ".arc-pill { padding: 1px 8px; border-radius: 9px; font-weight: bold; }\n"
        ".arc-dot { border-radius: 6px; min-width: 12px; min-height: 12px; }\n"
        ".arc-dot-on { background: #%06x; }\n"
        ".arc-dot-off { background: #808080; }\n",
        ARCSESSION_ACCENT_RGB, ARCSESSION_ACCENT_RGB,
        dark ? tint(ARCSESSION_ACCENT_RGB, 0.4) : ARCSESSION_ACCENT_DARK_RGB,
        ARCSESSION_ACCENT_RGB);
    gtk_css_provider_load_from_string(s_accent_css, buf);
}

static void on_dark_changed(AdwStyleManager *sm, GParamSpec *ps, gpointer ud)
{
    (void)ps; (void)ud;
    load_accent_css(adw_style_manager_get_dark(sm));
}

void arc_install_accent_css(void)
{
    AdwStyleManager *sm = adw_style_manager_get_default();
    if (s_accent_css)
        return;
    s_accent_css = gtk_css_provider_new();
    load_accent_css(adw_style_manager_get_dark(sm));
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
        GTK_STYLE_PROVIDER(s_accent_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect(sm, "notify::dark", G_CALLBACK(on_dark_changed), NULL);
}

/* ---- input ---------------------------------------------------------------- */

static guint32 keysym_of(guint keyval, guint keycode)
{
    /* the hardware key first; a keyval only for keys evdev has no HID
     * usage for (rare: media keys) */
    guint32 k = keycode >= 8 ? arcsession_keysym_from_evdev(keycode - 8) : 0;
    return k ? k : keyval;
}

void arc_window_run_sysaction(ArcWindow *self, int sa)
{
    switch (sa) {
    case ARC_SYSACT_RESET_CONFIG:
        arcsession_sysaction(self->session, sa);
        arc_window_toast(self, "Back to the FujiNet CONFIG client");
        break;
    case ARC_SYSACT_DEBUG_STOP:
        /* the debugger window attaches, which stops the machine */
        arc_debugger_show(GTK_WINDOW(self), self->session);
        break;
    default:
        break;
    }
}

/* An open dialog (Preferences, About, an alert) or menu has the keyboard:
 * Escape closes it and the arrows and Return move through it, instead of
 * driving the machine. The key capture sits on the window, ahead of them. */
static gboolean ui_has_keyboard(ArcWindow *self)
{
    GtkWidget *focus = gtk_root_get_focus(GTK_ROOT(self));
    if (adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(self)))
        return TRUE;
    return focus && gtk_widget_get_ancestor(focus, GTK_TYPE_POPOVER) != NULL;
}

gboolean arc_window_key(ArcWindow *self, guint keyval, guint keycode,
                       GdkModifierType state, gboolean down)
{
    guint32 keysym;
    int sa;

    if (!down) {
        if (keyval == GDK_KEY_F9 || keyval == GDK_KEY_F11 || keyval == GDK_KEY_F12)
            return TRUE;
        keysym = keysym_of(keyval, keycode);
        if (ui_has_keyboard(self)) {
            /* still let go of a machine key held into the dialog */
            arcsession_key(self->session, keysym, 0);
            return FALSE;
        }
        sa = arcsession_key_sysaction(self->session, keysym);
        if (sa >= 0) {
            self->sysact_down[sa] = FALSE;
            return TRUE;
        }
        return arcsession_key(self->session, keysym, 0) ? TRUE : FALSE;
    }

    if (ui_has_keyboard(self))
        return FALSE;

    /* The window's own keys, deliberately not bindable: they are how you
     * reach the panels that do the binding. */
    if (keyval == GDK_KEY_F9) {
        arc_controllers_toggle(GTK_WINDOW(self), self->session);
        return TRUE;
    }
    if (keyval == GDK_KEY_F12) {
        arc_debugger_toggle(GTK_WINDOW(self), self->session);
        return TRUE;
    }
    if (keyval == GDK_KEY_F11) {
        gtk_widget_activate_action(GTK_WIDGET(self), "win.fullscreen", NULL);
        return TRUE;
    }
    /* The menu accelerators are all Ctrl. Alt is not a modifier here: Left
     * Alt is a machine key (MAME's), and a game may want it held while
     * another key is pressed. */
    if (state & GDK_CONTROL_MASK)
        return FALSE;

    keysym = keysym_of(keyval, keycode);
    /* A system action is checked BEFORE the machine keys, so Escape always
     * gets back to CONFIG whatever else the key table says. Leading edge
     * only: GTK4 has no repeat flag. */
    sa = arcsession_key_sysaction(self->session, keysym);
    if (sa >= 0) {
        if (!self->sysact_down[sa]) {
            self->sysact_down[sa] = TRUE;
            arc_window_run_sysaction(self, sa);
        }
        return TRUE;
    }
    return arcsession_key(self->session, keysym, 1) ? TRUE : FALSE;
}

static gboolean on_key_pressed(GtkEventControllerKey *ctrl, guint keyval,
                               guint keycode, GdkModifierType state,
                               gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)ctrl;
    /* F1 is the console's RESET in the window itself, and Help while
     * Preferences or a dialog is open */
    if (keyval == GDK_KEY_F1 && ui_has_keyboard(self) &&
        !(state & GDK_MODIFIER_MASK & ~GDK_LOCK_MASK)) {
        arc_help_for(GTK_WINDOW(self));
        return TRUE;
    }
    return arc_window_key(self, keyval, keycode, state, TRUE);
}

static gboolean on_key_released(GtkEventControllerKey *ctrl, guint keyval,
                                guint keycode, GdkModifierType state,
                                gpointer user_data)
{
    (void)ctrl;
    return arc_window_key(user_data, keyval, keycode, state, FALSE);
}

void arc_window_release_keys(ArcWindow *self)
{
    arcsession_release_all(self->session);
    memset(self->sysact_down, 0, sizeof self->sysact_down);
}

/* Losing focus with keys held would leave the machine believing they are
 * still down. */
static void on_focus_leave(GtkEventControllerFocus *ctrl, gpointer user_data)
{
    (void)ctrl;
    arc_window_release_keys(user_data);
}

/* ---- the Console menu: labels that follow the bindings ----------------------- */

/* "Reset (F1)": the item's name and whatever key drives its target now (a
 * remap in the Controllers window shows here within a quarter second). The
 * mnemonics are unique across the whole menu. */
static void item_label(ArcWindow *self, char *dst, int dstsz, const char *name,
                       int target)
{
    arc_binding b = arcsession_binding_get(self->session, target);
    char key[32];
    if (b.keysym && arcsession_keysym_name(b.keysym, key, sizeof key) > 0)
        g_snprintf(dst, dstsz, "%s (%s)", name, key);
    else
        g_snprintf(dst, dstsz, "%s", name);
}

static void sync_console_menu(ArcWindow *self)
{
    char reset[96], all[256];

    item_label(self, reset, sizeof reset, "_Reset", ARC_TARGET_SWITCH(ARC_SW_RESET));
    g_snprintf(all, sizeof all, "%s", reset);

    if (strcmp(all, self->console_labels) == 0)
        return;
    g_strlcpy(self->console_labels, all, sizeof self->console_labels);
    g_menu_remove_all(self->console_menu);
    g_menu_append(self->console_menu, "_Pause", "win.pause");
    g_menu_append(self->console_menu, reset, "win.reset");
    g_menu_append(self->console_menu, "Reset to _CONFIG", "win.reset-config");
}

/* ---- messages and file pickers ---------------------------------------------------- */

static void open_cart_path(ArcWindow *self, const char *path);

/* A dialog for anything the user has to read: a toast would vanish before
 * the reason was read, and the reason is the whole point. */
static void show_message(GtkWidget *over, const char *heading, const char *body)
{
    AdwDialog *dlg = adw_alert_dialog_new(heading, body);
    adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dlg), "ok", "OK");
    adw_dialog_present(dlg, over);
}

void arc_window_message(ArcWindow *self, const char *heading, const char *body)
{
    show_message(GTK_WIDGET(self), heading, body);
}

static GtkFileDialog *file_dialog(const char *title, const char *filter_name,
                                  const char *const *suffixes, const char *folder)
{
    GtkFileDialog *dlg = gtk_file_dialog_new();
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter *images = gtk_file_filter_new();
    GtkFileFilter *all = gtk_file_filter_new();
    int i;

    /* suffixes match case-insensitively: GAME.BIN is as good as game.bin */
    gtk_file_filter_set_name(images, filter_name);
    for (i = 0; suffixes[i]; i++)
        gtk_file_filter_add_suffix(images, suffixes[i]);
    gtk_file_filter_set_name(all, "All files");
    gtk_file_filter_add_pattern(all, "*");
    g_list_store_append(filters, images);
    g_list_store_append(filters, all);
    gtk_file_dialog_set_title(dlg, title);
    gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));
    if (folder && *folder) {
        g_autoptr(GFile) dir = g_file_new_for_path(folder);
        gtk_file_dialog_set_initial_folder(dlg, dir);
    }
    g_object_unref(images);
    g_object_unref(all);
    g_object_unref(filters);
    return dlg;
}

static const char *const k_cart_exts[] = { "bin", "rom", "arc", "zip", NULL };

/* The gamepad thread cannot call into GTK; it posts system actions and this
 * timer takes them. It also watches for gamepads coming and going, and says
 * so: a pad that drops off Bluetooth mid-game should not be a mystery. */
static gboolean sysact_drain_tick(gpointer user_data)
{
    ArcWindow *self = user_data;
    GAction *pause = g_action_map_lookup_action(G_ACTION_MAP(self), "pause");
    const gboolean paused = arcsession_is_paused(self->session) != 0;
    unsigned gen;
    int sa;
    while (arcsession_sysaction_take(self->session, &sa))
        arc_window_run_sysaction(self, sa);

    gen = arcsession_gamepad_generation(self->session);
    if (gen != self->pad_generation) {
        char text[160];
        self->pad_generation = gen;
        if (arcsession_gamepad_last_event(self->session, text, sizeof text) > 0)
            arc_window_toast(self, text);
    }

    sync_console_menu(self);
    /* the session pauses too (the debugger's Run/Stop is separate): the menu's
     * check follows it */
    if (pause) {
        g_autoptr(GVariant) state = g_action_get_state(pause);
        if (g_variant_get_boolean(state) != paused)
            g_simple_action_set_state(G_SIMPLE_ACTION(pause), g_variant_new_boolean(paused));
    }
    return G_SOURCE_CONTINUE;
}

/* ---- status --------------------------------------------------------------- */

static gboolean update_status(gpointer user_data)
{
    ArcWindow *self = user_data;
    char text[1200], st[160];
    gboolean on = FALSE;

    if (!arcsession_is_running(self->session)) {
        g_snprintf(text, sizeof text, "Stopped");
    } else {
        const char *cart = arcsession_cart_path(self->session);
        const char *slash = cart ? strrchr(cart, '/') : NULL;
        int link = arcsession_cart_link_up(self->session);
        arcsession_cart_status(self->session, st, sizeof st);
        on = link == 1;
        if (cart && *cart)
            g_snprintf(text, sizeof text, "%s \xe2\x80\x94 FujiNet %s",
                       slash ? slash + 1 : cart, st);
        else
            g_snprintf(text, sizeof text, "FujiNet %s", st);
        if (arcsession_is_paused(self->session)) {
            char paused[1200];
            g_snprintf(paused, sizeof paused, "Paused \xe2\x80\x94 %s", text);
            g_strlcpy(text, paused, sizeof text);
        }
    }
    gtk_label_set_text(GTK_LABEL(self->status), text);
    if (on) {
        gtk_widget_add_css_class(self->status_dot, "arc-dot-on");
        gtk_widget_remove_css_class(self->status_dot, "arc-dot-off");
    } else {
        gtk_widget_add_css_class(self->status_dot, "arc-dot-off");
        gtk_widget_remove_css_class(self->status_dot, "arc-dot-on");
    }
    return G_SOURCE_CONTINUE;
}

/* ---- cartridges --------------------------------------------------------------- */

static const char *base_name(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* A cartridge the console cannot map says why in a dialog. */
static void open_cart_path(ArcWindow *self, const char *path)
{
    char msg[1200];
    if (arcsession_load_cart(self->session, path) != 0) {
        show_message(GTK_WIDGET(self), "Cannot Open Cartridge",
                     arcsession_last_error(self->session));
        return;
    }
    g_snprintf(msg, sizeof msg, "Running %s", base_name(path));
    arc_window_toast(self, msg);
}

/* A dropped file: a cartridge runs, anything else goes to FujiNet's SD
 * folder. */
void arc_window_load_media(ArcWindow *self, const char *path)
{
    char dest[1024];

    if (arcsession_media_is_cartridge(path)) {
        open_cart_path(self, path);
        return;
    }
    if (arcsession_import_media(self->session, path, dest, sizeof dest) != 0) {
        arc_window_toast(self, arcsession_last_error(self->session));
        return;
    }
    arc_window_toast(self, "Copied to FujiNet's SD folder \xe2\x80\x94 mount it "
                           "from the CONFIG client");
}

static GtkFileDialog *cart_dialog(ArcWindow *self, const char *title)
{
    const char *cart = arcsession_cart_path(self->session);
    g_autofree char *dir = cart && *cart ? g_path_get_dirname(cart) : NULL;
    /* where the last cartridge came from: the next one is usually beside it */
    return file_dialog(title, "Arcadia 2001 cartridges (*.bin, *.rom, *.arc, *.zip)", k_cart_exts, dir);
}

static void on_cart_chosen(GObject *src, GAsyncResult *res, gpointer user_data)
{
    ArcWindow *self = user_data;
    g_autoptr(GFile) file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, NULL);
    g_autofree char *path = NULL;
    if (!file) return;
    path = g_file_get_path(file);
    if (path) open_cart_path(self, path);
}

static void action_open(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    GtkFileDialog *dlg = cart_dialog(self, "Open Cartridge");
    (void)a; (void)p;
    gtk_file_dialog_open(dlg, GTK_WINDOW(self), NULL, on_cart_chosen, self);
    g_object_unref(dlg);
}

static void on_sd_chosen(GObject *src, GAsyncResult *res, gpointer user_data)
{
    ArcWindow *self = user_data;
    g_autoptr(GFile) file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, NULL);
    g_autofree char *path = NULL;
    char dest[1024], msg[1200];
    if (!file) return;
    path = g_file_get_path(file);
    if (!path) return;
    if (arcsession_import_cart_to_sd(self->session, path, dest, sizeof dest) != 0) {
        arc_window_toast(self, arcsession_last_error(self->session));
        return;
    }
    g_snprintf(msg, sizeof msg, "Copied to %s \xe2\x80\x94 boot it from the "
               "CONFIG client", dest);
    arc_window_toast(self, msg);
}

static void action_import_sd(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    GtkFileDialog *dlg = cart_dialog(self, "Import Cartridge to SD");
    (void)a; (void)p;
    gtk_file_dialog_open(dlg, GTK_WINDOW(self), NULL, on_sd_chosen, self);
    g_object_unref(dlg);
}

static void action_eject(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    arcsession_eject(self->session);
    arc_window_toast(self, "Cartridge ejected \xe2\x80\x94 back to CONFIG");
}

static gboolean on_drop(GtkDropTarget *t, const GValue *value, double x,
                        double y, gpointer user_data)
{
    ArcWindow *self = user_data;
    g_autofree char *path = NULL;
    (void)t; (void)x; (void)y;
    if (!G_VALUE_HOLDS(value, G_TYPE_FILE)) return FALSE;
    path = g_file_get_path(G_FILE(g_value_get_object(value)));
    if (!path) return FALSE;
    arc_window_load_media(self, path);
    return TRUE;
}

/* ---- the console and actions ---------------------------------------------- */

static void action_pause(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    g_autoptr(GVariant) state = g_action_get_state(G_ACTION(a));
    gboolean on = !g_variant_get_boolean(state);
    (void)p;
    g_simple_action_set_state(a, g_variant_new_boolean(on));
    arcsession_set_paused(self->session, on);
    update_status(self);
}

/* The console's RESET key from the menu: the session holds it for a few
 * frames, long enough for the console to see it (it samples the keys once a
 * frame). It resets the 2650 only: a FujiNet app restarts from the
 * cartridge's current image. */
static void action_reset(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    arcsession_reset(self->session);
}

static void action_reset_config(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    (void)a; (void)p;
    arc_window_run_sysaction(user_data, ARC_SYSACT_RESET_CONFIG);
}

static void action_controllers(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    arc_controllers_toggle(GTK_WINDOW(self), self->session);
}

static void action_debugger(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    arc_debugger_toggle(GTK_WINDOW(self), self->session);
}

static void action_fullscreen(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    self->fullscreen = !self->fullscreen;
    if (self->fullscreen) gtk_window_fullscreen(GTK_WINDOW(self));
    else gtk_window_unfullscreen(GTK_WINDOW(self));
}

static void action_fujinet_config(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    if (arcsession_fujinet_external(self->session)) {
        arc_window_toast(self, "FujiNet is external: use its own web UI");
        return;
    }
    if (!arcsession_fujinet_running(self->session)) {
        arc_window_toast(self, "FujiNet is not running");
        return;
    }
    arc_fujiconfig_show(GTK_WINDOW(self), self->session);
}

static void action_fujinet_log(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    if (arcsession_fujinet_external(self->session)) {
        arc_window_toast(self, "FujiNet is external: see its own console");
        return;
    }
    arc_fujilog_show(GTK_WINDOW(self), self->session);
}

/* Power-cycle onto the settings store's options (a full restart if FujiNet,
 * audio or the gamepads were switched). Preferences hands this to
 * arc_prefs_show() as its close callback. */
static void restart_session(ArcWindow *self)
{
    if (arcsession_restart(self->session) != 0) {
        show_message(GTK_WIDGET(self), "The Console Did Not Start",
                     arcsession_last_error(self->session));
        return;
    }
    arc_window_toast(self, "Machine options applied (power cycled)");
}

static void action_prefs(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    (void)a; (void)p;
    arc_prefs_show(self, self->session, restart_session);
}

static void action_quit(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    (void)a; (void)p;
    gtk_window_close(GTK_WINDOW(user_data));
}

static void action_about(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    AdwDialog *about;
    (void)a; (void)p;
    about = adw_about_dialog_new();
    adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about), "FujiNet Go Arcadia 2001");
    adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about), arc_icon_name());
    adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about), ARC_VERSION_STRING);
    adw_about_dialog_set_developer_name(ADW_ABOUT_DIALOG(about), "Thomas Cherryhomes");
    adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about), "https://fujinet.online/");
    adw_about_dialog_set_issue_url(ADW_ABOUT_DIALOG(about),
        "https://github.com/FujiNetWIFI/fujinet-go-arcadia-desktop/issues");
    adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about), GTK_LICENSE_GPL_3_0);
    adw_about_dialog_set_comments(ADW_ABOUT_DIALOG(about),
        "Emerson Arcadia 2001 (and its clones) with a built-in FujiNet cartridge. The "
        "emulator is MAME's Arcadia 2001 (Signetics 2650 and 2637 UVI; BSD-3-Clause "
        "and GPL-2.0-or-later) transposed to C; the FujiNet firmware and the "
        "cartridge are by Thomas Cherryhomes and the FujiNet contributors "
        "(GPL-3.0-or-later).");
    adw_dialog_present(about, GTK_WIDGET(self));
}

static void action_aspect(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    g_autoptr(GVariant) state = g_action_get_state(G_ACTION(a));
    gboolean tv = !g_variant_get_boolean(state);
    (void)p;
    g_simple_action_set_state(a, g_variant_new_boolean(tv));
    arc_display_set_tv_aspect(ARC_DISPLAY(self->display), tv);
    /* "aspect": 0 = the television's 4:3, 1 = square pixels */
    arcsession_set_int(self->session, "aspect", tv ? 0 : 1);
}

static void action_smooth(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    ArcWindow *self = user_data;
    g_autoptr(GVariant) state = g_action_get_state(G_ACTION(a));
    gboolean sm = !g_variant_get_boolean(state);
    (void)p;
    g_simple_action_set_state(a, g_variant_new_boolean(sm));
    arc_display_set_smooth(ARC_DISPLAY(self->display), sm);
    arcsession_set_int(self->session, "smooth", sm ? 1 : 0);
}

void arc_window_apply_aspect(ArcWindow *self, int aspect)
{
    GAction *a = g_action_map_lookup_action(G_ACTION_MAP(self), "tv-aspect");
    arc_display_set_tv_aspect(ARC_DISPLAY(self->display), aspect == 0);
    if (a)
        g_simple_action_set_state(G_SIMPLE_ACTION(a), g_variant_new_boolean(aspect == 0));
}

void arc_window_apply_smooth(ArcWindow *self, gboolean smooth)
{
    GAction *a = g_action_map_lookup_action(G_ACTION_MAP(self), "smooth");
    arc_display_set_smooth(ARC_DISPLAY(self->display), smooth);
    if (a)
        g_simple_action_set_state(G_SIMPLE_ACTION(a), g_variant_new_boolean(smooth));
}

const char *arc_window_help_context(ArcWindow *self)
{
    static char ctx[64];
    AdwDialog *d = adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(self));

    if (d && ADW_IS_PREFERENCES_DIALOG(d)) {
        const char *page = adw_preferences_dialog_get_visible_page_name(ADW_PREFERENCES_DIALOG(d));
        if (page) {
            g_snprintf(ctx, sizeof ctx, "prefs/%s", page);
            return ctx;
        }
    }
    return "main";
}

/* app.help: every window's Ctrl+Shift+H, for what it shows. Not a cast of
 * the active window: it can be the debugger, the controllers or the log. */
static void action_help(GSimpleAction *a, GVariant *p, gpointer user_data)
{
    (void)a; (void)p;
    arc_help_for(gtk_application_get_active_window(GTK_APPLICATION(user_data)));
}

static const GActionEntry app_actions[] = {
    { "help", action_help, NULL, NULL, NULL, { 0 } },
};

static const GActionEntry win_actions[] = {
    { "open", action_open, NULL, NULL, NULL, { 0 } },
    { "eject", action_eject, NULL, NULL, NULL, { 0 } },
    { "import-sd", action_import_sd, NULL, NULL, NULL, { 0 } },
    { "pause", action_pause, NULL, "false", NULL, { 0 } },
    { "reset", action_reset, NULL, NULL, NULL, { 0 } },
    { "reset-config", action_reset_config, NULL, NULL, NULL, { 0 } },
    { "controllers", action_controllers, NULL, NULL, NULL, { 0 } },
    { "debugger", action_debugger, NULL, NULL, NULL, { 0 } },
    { "fullscreen", action_fullscreen, NULL, NULL, NULL, { 0 } },
    { "tv-aspect", action_aspect, NULL, "true", NULL, { 0 } },
    { "smooth", action_smooth, NULL, "false", NULL, { 0 } },
    { "fujinet-config", action_fujinet_config, NULL, NULL, NULL, { 0 } },
    { "fujinet-log", action_fujinet_log, NULL, NULL, NULL, { 0 } },
    { "prefs", action_prefs, NULL, NULL, NULL, { 0 } },
    { "about", action_about, NULL, NULL, NULL, { 0 } },
    { "quit", action_quit, NULL, NULL, NULL, { 0 } },
};

/* ---- construction --------------------------------------------------------- */

static GMenu *build_menu(ArcWindow *self)
{
    GMenu *menu = g_menu_new();
    GMenu *cart = g_menu_new();
    GMenu *view = g_menu_new();
    GMenu *fuji = g_menu_new();
    GMenu *app = g_menu_new();

    g_menu_append(cart, "_Open Cartridge\xe2\x80\xa6", "win.open");
    g_menu_append(cart, "E_ject Cartridge", "win.eject");
    g_menu_append(cart, "Import Cartridge to _SD\xe2\x80\xa6", "win.import-sd");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(cart));

    /* filled (and kept current) by sync_console_menu */
    self->console_menu = g_menu_new();
    g_menu_append_section(menu, "Console", G_MENU_MODEL(self->console_menu));

    g_menu_append(view, "Con_trollers (F9)", "win.controllers");
    g_menu_append(view, "_Debugger (F12)", "win.debugger");
    g_menu_append(view, "T_V Aspect", "win.tv-aspect");
    g_menu_append(view, "S_mooth Scaling", "win.smooth");
    g_menu_append(view, "F_ullscreen (F11)", "win.fullscreen");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(view));

    g_menu_append(fuji, "FujiNet _Web UI", "win.fujinet-config");
    g_menu_append(fuji, "Console _Log", "win.fujinet-log");
    g_menu_append_section(menu, "FujiNet", G_MENU_MODEL(fuji));

    g_menu_append(app, "Pr_eferences", "win.prefs");
    g_menu_append(app, "_Help", "app.help");
    g_menu_append(app, "_About FujiNet Go Arcadia 2001", "win.about");
    g_menu_append(app, "_Quit", "win.quit");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(app));

    g_object_unref(cart);
    g_object_unref(view);
    g_object_unref(fuji);
    g_object_unref(app);
    return menu;
}

static void arc_window_dispose(GObject *object)
{
    ArcWindow *self = ARC_WINDOW(object);
    if (self->status_id) {
        g_source_remove(self->status_id);
        self->status_id = 0;
    }
    if (self->sysact_id) {
        g_source_remove(self->sysact_id);
        self->sysact_id = 0;
    }
    g_clear_object(&self->console_menu);
    G_OBJECT_CLASS(arc_window_parent_class)->dispose(object);
}

static void arc_window_class_init(ArcWindowClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = arc_window_dispose;
}

static void arc_window_init(ArcWindow *self)
{
    (void)self;
}

GtkWidget *arc_window_new(AdwApplication *app, arcsession *session)
{
    ArcWindow *self = g_object_new(ARC_TYPE_WINDOW, "application", app, NULL);
    GtkWidget *header, *menu_button, *toolbar, *status_box;
    GtkEventController *keys, *focus;
    g_autoptr(GMenu) menu = NULL;

    self->session = session;
    arc_install_accent_css();

    gtk_window_set_title(GTK_WINDOW(self), "FujiNet Go Arcadia 2001");
    gtk_window_set_icon_name(GTK_WINDOW(self), arc_icon_name());
    /* The picture at the television's 4:3, 3x the 262-line frame's height
     * (786) wide at 4:3 = 1048 x 786 under the header bar. */
    gtk_window_set_default_size(GTK_WINDOW(self), 1048, 786 + 46);

    g_action_map_add_action_entries(G_ACTION_MAP(self), win_actions,
                                    G_N_ELEMENTS(win_actions), self);
    if (!g_action_map_lookup_action(G_ACTION_MAP(app), "help"))
        g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions,
                                        G_N_ELEMENTS(app_actions), app);
    {
        static const char *const prefs_accels[] = { "<Control>comma", NULL };
        static const char *const open_accels[] = { "<Control>o", NULL };
        static const char *const reboot_accels[] = { "<Control>r", NULL };
        static const char *const quit_accels[] = { "<Control>q", NULL };
        static const char *const help_accels[] = { "<Control><Shift>h", NULL };
        gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.help", help_accels);
        gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.prefs", prefs_accels);
        gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.open", open_accels);
        gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.reset-config", reboot_accels);
        gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.quit", quit_accels);
    }

    header = adw_header_bar_new();
    menu = build_menu(self);
    menu_button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));
    gtk_menu_button_set_primary(GTK_MENU_BUTTON(menu_button), TRUE);
    gtk_widget_set_tooltip_text(menu_button, "Main Menu");
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), menu_button);

    status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    self->status_dot = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(self->status_dot, "arc-dot");
    gtk_widget_add_css_class(self->status_dot, "arc-dot-off");
    gtk_widget_set_valign(self->status_dot, GTK_ALIGN_CENTER);
    self->status = gtk_label_new("Starting...");
    gtk_widget_add_css_class(self->status, "dim-label");
    gtk_label_set_ellipsize(GTK_LABEL(self->status), PANGO_ELLIPSIZE_MIDDLE);
    gtk_box_append(GTK_BOX(status_box), self->status_dot);
    gtk_box_append(GTK_BOX(status_box), self->status);
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), status_box);

    self->display = arc_display_new(session);
    arc_display_set_tv_aspect(ARC_DISPLAY(self->display),
        arcsession_get_int(session, "aspect", 0) == 0);
    arc_display_set_smooth(ARC_DISPLAY(self->display),
        arcsession_get_int(session, "smooth", 0) != 0);
    {
        GAction *a = g_action_map_lookup_action(G_ACTION_MAP(self), "tv-aspect");
        g_simple_action_set_state(G_SIMPLE_ACTION(a),
            g_variant_new_boolean(arcsession_get_int(session, "aspect", 0) == 0));
        a = g_action_map_lookup_action(G_ACTION_MAP(self), "smooth");
        g_simple_action_set_state(G_SIMPLE_ACTION(a),
            g_variant_new_boolean(arcsession_get_int(session, "smooth", 0) != 0));
    }

    {
        GtkDropTarget *drop = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
        g_signal_connect(drop, "drop", G_CALLBACK(on_drop), self);
        gtk_widget_add_controller(self->display, GTK_EVENT_CONTROLLER(drop));
    }

    self->toast_overlay = adw_toast_overlay_new();
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(self->toast_overlay), self->display);

    toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), self->toast_overlay);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), toolbar);

    /* Capture on the WINDOW so input works no matter what has focus. */
    keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
    g_signal_connect(keys, "key-pressed", G_CALLBACK(on_key_pressed), self);
    g_signal_connect(keys, "key-released", G_CALLBACK(on_key_released), self);
    gtk_widget_add_controller(GTK_WIDGET(self), keys);

    focus = gtk_event_controller_focus_new();
    g_signal_connect(focus, "leave", G_CALLBACK(on_focus_leave), self);
    gtk_widget_add_controller(GTK_WIDGET(self), focus);

    self->status_id = g_timeout_add_seconds(1, update_status, self);
    /* The current pad set is the baseline: no toast for pads that were
     * already plugged in at launch. */
    self->pad_generation = arcsession_gamepad_generation(session);
    self->sysact_id = g_timeout_add(250, sysact_drain_tick, self);
    sync_console_menu(self);
    update_status(self);

    /* ARC_OPEN_CONTROLLERS=1 / ARC_OPEN_DEBUGGER=1 / ARC_OPEN_SETTINGS=1
     * open those windows at launch, following the family's convention: the
     * way in when the app misbehaves before the menu is reachable. */
    {
        const char *env = g_getenv("ARC_OPEN_CONTROLLERS");
        if (env && *env && *env != '0')
            arc_controllers_toggle(GTK_WINDOW(self), session);
        env = g_getenv("ARC_OPEN_DEBUGGER");
        if (env && *env && *env != '0')
            arc_debugger_show(GTK_WINDOW(self), session);
        env = g_getenv("ARC_OPEN_SETTINGS");
        if (env && *env && *env != '0')
            arc_prefs_show(self, session, restart_session);
    }
    return GTK_WIDGET(self);
}
