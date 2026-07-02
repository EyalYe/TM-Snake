# TaskMaster-C3 — app project template

Build your own apps for a **TaskMaster-C3** device (1.3" OLED + rotary encoder + two
buttons). **Fork this repo** — it's your project. The OS (`taskmaster_core`) is a
**sealed dependency** you never fork or edit; you only add apps.

## The one file you edit: [`apps.yaml`](apps.yaml)

```yaml
core:                                   # the sealed OS — pin a version, never edit it
  git: git@github.com:EyalYe/TaskMaster.git
  path: components/taskmaster_core
  version: main
apps:                                   # YOUR apps
  - name: app_skeleton
    path: apps/app_skeleton
```

A build hook compiles `apps.yaml` into `main/idf_component.yml` (generated, git-ignored)
— you never touch that, `main/`, or the core.

## Write an app

Copy [`apps/app_skeleton`](apps/app_skeleton) to a new folder, rename it, and add an
entry in `apps.yaml`. An app fills in a `device_app_t` (`init` / `on_event` / `render`
/ `exit`) and self-registers. Full contract: **[`docs/APP_API.md`](docs/APP_API.md)**.

## Example apps

Real, working apps built exactly this way — read them, or use one as a starting point:

- **[TM-Pomodoro](https://github.com/EyalYe/TM-Pomodoro)** — a **full-screen graphical**
  work/break timer (two shrinking circles, no hint bar). Shows raw LVGL drawing, the
  live `tick_ms`, and long-press Select. *(pins core `v1.2.0`)*
- **[TM-ToDo](https://github.com/EyalYe/TM-ToDo)** — **two apps sharing one library**,
  pick one: Todoist (Cloud) or a LAN server (Local), both over a shared `todo_common`
  (offline cache + write-replay). Shows a shared component + app selection in `apps.yaml`.
- **`apps/app_skeleton`** (in this repo) — the minimal starting point.

## Build it

Two ways — you don't need a toolchain if you use CI:

- **CI (no toolchain):** push. **GitHub Actions** builds the firmware `.bin` and
  attaches it to a **Release**. *(`.github/workflows/build.yml`)*
- **Local:** with ESP-IDF installed, `idf.py set-target esp32c3 && idf.py build`.

## Flash + update — local tools, nothing hosted

Nothing is hosted in the cloud. Two small Python tools live in [`tools/`](tools) — they
only need `pip install esptool` (no ESP-IDF):

- **First flash (USB):**
  ```
  python tools/flash.py                    # flashes ./build (after idf.py build)
  python tools/flash.py --bin firmware.bin # or a merged image from a Release
  ```
  Auto-detects the port. If the device won't connect, hold **BOOT**, tap **RESET**,
  release **BOOT** (download mode), then retry.

- **Updates (OTA over your LAN):**
  ```
  python tools/ota_serve.py                # serves ./build/<app>.bin over HTTP
  ```
  It prints a URL like `http://<your-machine>:8000/<app>.bin`. Point the device's
  **`fw_url`** at it (Settings → Web config), then Settings → **Check update** — the
  device downloads the new image over Wi-Fi and reboots into it. You host nothing.

## What the device does

The OS gives you: a Launcher, provisioning (Wi-Fi + your app's config from a phone),
a Settings hub, a status bar (time · weather · connectivity), offline caching, and OTA
— all provided by `taskmaster_core`. Your app just draws and reacts. See the core repo
for the full picture.
