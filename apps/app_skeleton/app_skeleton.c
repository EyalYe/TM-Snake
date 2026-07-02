/*
 * app_skeleton — your starting point. Copy this folder, rename it, and build your app.
 *
 * An app is just a self-contained component that fills in a `device_app_t` and
 * registers itself. The OS (taskmaster_core) calls your lifecycle hooks on one UI
 * task — no threads, no mutexes. You only use the public headers below; you never
 * touch the core. See docs/APP_API.md for the full contract.
 */
#include "app.h"          /* device_app_t + TASKMASTER_REGISTER_APP */
#include "input.h"        /* EV_* input events */
#include "ui_frame.h"     /* ui_text_row / ui_frame_content / control_hints_t */
#include "app_store.h"    /* your own private, persistent key/value store */
#include "app_config.h"   /* declare config fields collected at setup / in Settings */

#include <stdio.h>

/* Require the app-API version this app is written against. Compatible = same major,
 * minor >= 1.0. If the core pinned in apps.yaml is incompatible, the build fails with
 * a clear message (rather than a cryptic compile error). See docs/APP_API.md §11. */
TASKMASTER_REQUIRE_API(1, 0);

/* Right-hand hint bar: what the buttons do on this screen. Encoder rotation always
 * scrolls, so it isn't shown. "OPN"/"DON"/"MNU"/"RST"/"BAK"/"OK" render as glyphs;
 * any other <=3-char text shows as-is. */
static const control_hints_t HINTS = { .click = "RST", .select = "RST" };

/* A config field the setup form / Settings collects into our own namespace ("skel").
 * ACFG_PASTE = typed once in the browser form; ACFG_KNOB = adjustable in Settings. */
#define SKEL_NAME_MAX 24
static const app_cfg_field_t CFG[] = {
    { .key = "name", .label = "Your name", .type = ACFG_STR, .input = ACFG_PASTE, .max_len = SKEL_NAME_MAX },
};
TASKMASTER_REGISTER_APP_CONFIG("skel", "My App", CFG);

#define TITLE_ROW  0
#define HINT_ROW   1
#define COUNT_ROW  2
#define SKEL_LINE_MAX   24

static int         s_counter;
static app_store_t s_store;    /* our own NVS namespace — survives reboot */

/* Called once when the app is opened. Allocate/restore state here. */
static void skel_init(void)
{
    app_store_open(&s_store, "skel");
    uint32_t saved = 0;
    app_store_get_u32(&s_store, "count", &saved, 0);
    s_counter = (int)saved;
}

/* Mutate state on input; the OS calls render() afterward. */
static void skel_on_event(uint8_t ev)
{
    switch (ev) {
    case EV_ENCODER_CW:  s_counter++; break;
    case EV_ENCODER_CCW: s_counter--; break;
    case EV_ENCODER_CLICK:
    case EV_SELECT:      s_counter = 0; break;   /* push / Select resets */
    default: break;
    }
}

/* Draw the screen. Clear the content, add your widgets, then publish the hints. */
static void skel_render(void)
{
    lv_obj_clean(ui_frame_content());

    char name[SKEL_NAME_MAX + 1] = {0};
    app_store_get_str(&s_store, "name", name, sizeof(name), "");
    if (name[0]) {
        char greet[8 + SKEL_NAME_MAX];
        snprintf(greet, sizeof(greet), "Hi, %s", name);
        ui_text_row(TITLE_ROW, greet);
    } else {
        ui_text_row(TITLE_ROW, "My App");
    }

    ui_text_row(HINT_ROW, "Turn the knob:");
    char line[SKEL_LINE_MAX];
    snprintf(line, sizeof(line), " count = %d", s_counter);
    ui_text_row(COUNT_ROW, line);

    ui_frame_set_hints(&HINTS);
}

/* Called on the way out (Home, or switching apps). Persist + release everything —
 * teardown must be total (the OS reuses the screen for the next app). */
static void skel_exit(void)
{
    app_store_set_u32(&s_store, "count", (uint32_t)s_counter);
    app_store_close(&s_store);
}

static const device_app_t skeleton_app = {
    .name     = "My App",          /* shown in the Launcher */
    .init     = skel_init,
    .on_event = skel_on_event,
    .render   = skel_render,
    .exit     = skel_exit,
};

TASKMASTER_REGISTER_APP(skeleton_app);
