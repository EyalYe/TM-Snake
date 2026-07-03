#!/usr/bin/env python3
"""build.py — clean, one-command firmware build for a TaskMaster-C3 project.

Wipes stale build state first so nothing old can linger — a previous `build/`, cached
`managed_components/` (old core/app versions), a pinned `dependencies.lock`, the
generated manifest — then does a fresh configure + build. Run it whenever you've edited
apps.yaml, pulled a newer core/app, or just want a guaranteed-clean image.

Requires ESP-IDF activated in the shell first:
    . $HOME/esp/esp-idf/export.sh          # or wherever your ESP-IDF lives

    python tools/build.py                   # clean + build
    python tools/build.py --flash           # ... then flash it over USB
    python tools/build.py --keep-config      # keep sdkconfig (don't reset menuconfig tweaks)

Output (both under build/): firmware-merged.bin (first USB flash) + taskmaster_c3_app.bin (OTA).
"""
import argparse
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Everything that can hold stale state — all regenerated / re-fetched by a fresh build.
CLEAN = ["build", "managed_components", "dependencies.lock", "main/idf_component.yml"]


def _rm(rel):
    p = os.path.join(ROOT, rel)
    if os.path.isdir(p):
        shutil.rmtree(p, ignore_errors=True)
    elif os.path.exists(p):
        os.remove(p)


def _run(*cmd):
    print("+ " + " ".join(cmd), flush=True)
    subprocess.run(cmd, cwd=ROOT, check=True)


def main():
    ap = argparse.ArgumentParser(description="Clean one-command build for TaskMaster-C3.")
    ap.add_argument("--flash", action="store_true", help="flash over USB after building")
    ap.add_argument("--keep-config", action="store_true", help="keep sdkconfig")
    args = ap.parse_args()

    if not os.environ.get("IDF_PATH"):
        sys.exit("ESP-IDF isn't activated. Run your install's export.sh first, e.g.\n"
                 "    . $HOME/esp/esp-idf/export.sh")

    print("Cleaning stale build state ...")
    for rel in CLEAN:
        _rm(rel)
    if not args.keep_config:
        _rm("sdkconfig")

    _run("idf.py", "set-target", "esp32c3")     # configure: compose apps.yaml + fetch deps
    # the managed lvgl component ships a stray CMakePresets.json that breaks the build
    _rm("managed_components/lvgl__lvgl/CMakePresets.json")
    _run("idf.py", "build")
    _run("idf.py", "merge-bin", "-o", os.path.join("build", "firmware-merged.bin"))

    print("\nDone:")
    print("  build/firmware-merged.bin    -> first USB flash  (python tools/flash.py --bin ...)")
    print("  build/taskmaster_c3_app.bin  -> OTA update        (python tools/ota_serve.py --bin ...)")
    if args.flash:
        _run(sys.executable, os.path.join("tools", "flash.py"))   # flashes ./build over USB


if __name__ == "__main__":
    main()
