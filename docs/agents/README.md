# Agent onboarding (app project) — start here

This folder is the **complete context for an AI agent (or contributor)** building apps on
TaskMaster-C3 in **this fork**. You write **apps**; the OS (`taskmaster_core`) is a **sealed git
dependency** you never fork or edit. Read this before making changes.

## Read order

1. **[CONTEXT.md](CONTEXT.md)** — what the device/project is, the repo topology, and how apps fit in.
2. **[PREFERENCES.md](PREFERENCES.md)** — the owner's standards + how they like to work. Follow these.
3. **[skills/](skills/)** — task recipes:
   - [add-an-app.md](skills/add-an-app.md) — **the main one**: write/register an app, the public API
   - [build-and-flash.md](skills/build-and-flash.md) — build this project, flash, serial, gotchas
   - [onboarding-and-ota.md](skills/onboarding-and-ota.md) — CI, `flash.py`, `ota_serve.py`, LAN OTA
   - [debugging.md](skills/debugging.md) — serial capture, resets, provisioning

The full app contract is **[../APP_API.md](../APP_API.md)** — read it before writing an app.

## The rules that matter here

1. **Never touch the core.** `taskmaster_core` is a sealed contract. You use the public headers
   (`app.h`, `ui_frame.h`, `ui_list.h`, `app_store.h`, `net_status.h`, `async_job.h`, `app_config.h`)
   and nothing else. If a capability isn't exposed, it's off-limits — don't reach in.
2. **The only file you edit to add/remove apps is `apps.yaml`.** Everything downstream
   (`main/idf_component.yml`) is generated.
3. **No magic numbers** — named `#define`/enum in a header.
4. **Verify on hardware, report honestly.**
5. **Teardown is total** — an app's `exit()` frees everything (the OS reuses the screen).

## Note

This is a copy of the core project's agent knowledge base, trimmed to the app-author scope (the
`core-development` and `glyphs` skills are core-internal and intentionally omitted — apps don't do
those). The authoritative, full version lives in the core repo (`TaskMaster/docs/agents/`).
