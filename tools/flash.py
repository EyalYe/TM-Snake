#!/usr/bin/env python3
"""flash.py — first-time USB flash for a TaskMaster-C3 device (no ESP-IDF needed).

USE THIS FOR THE FIRST FLASH over USB. The right file is **firmware-merged.bin** (the
whole image) — NOT taskmaster_c3_app.bin (that one is for OTA; see tools/ota_serve.py).

Only needs Python + esptool (`pip install esptool`) and a USB-C **data** cable.

    python tools/flash.py                             # local build: auto-flash ./build
    python tools/flash.py --bin firmware-merged.bin   # a downloaded CI/Release image
    python tools/flash.py --port /dev/cu.usbmodemXXXX --bin firmware-merged.bin

If the device isn't detected / won't connect: hold **BOOT**, tap **RESET**, release
**BOOT** (download mode), then retry.
"""
import argparse
import glob
import os
import subprocess
import sys

CHIP = "esp32c3"
BAUD = "460800"
# Multi-file (local build) layout — offsets match partitions.csv / `idf.py flash`.
BUILD_PARTS = [
    ("0x0",     "bootloader/bootloader.bin"),
    ("0x8000",  "partition_table/partition-table.bin"),
    ("0xf000",  "ota_data_initial.bin"),
    ("0x20000", "taskmaster_c3_app.bin"),
]


def autodetect_port():
    cands = sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/cu.usbserial*") +
                   glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))
    return cands[0] if cands else None


def main():
    ap = argparse.ArgumentParser(description="First-time USB flash for TaskMaster-C3.")
    ap.add_argument("--port", help="serial port (auto-detected if omitted)")
    ap.add_argument("--bin", help="a single merged image to flash at 0x0 (else ./build)")
    ap.add_argument("--build-dir", default="build", help="build dir for the multi-file layout")
    args = ap.parse_args()

    port = args.port or autodetect_port()
    if not port:
        sys.exit("No serial port found. Plug in the device with a DATA cable, or pass --port.\n"
                 "If nothing shows: hold BOOT, tap RESET, release BOOT (download mode).")

    esptool = [sys.executable, "-m", "esptool", "--chip", CHIP, "-b", BAUD, "--port", port,
               "--before", "default-reset", "--after", "hard-reset", "write-flash",
               "--flash-mode", "dio", "--flash-size", "4MB", "--flash-freq", "80m"]

    if args.bin:
        if not os.path.exists(args.bin):
            sys.exit("not found: %s" % args.bin)
        cmd = esptool + ["0x0", args.bin]
    else:
        cmd = list(esptool)
        for off, rel in BUILD_PARTS:
            p = os.path.join(args.build_dir, rel)
            if not os.path.exists(p):
                sys.exit("missing %s — run `idf.py build` first, or pass --bin <merged.bin>." % p)
            cmd += [off, p]

    print("Flashing %s via %s ..." % (args.bin or args.build_dir + "/", port))
    try:
        subprocess.run(cmd, check=True)
    except FileNotFoundError:
        sys.exit("esptool not found. Install it:  pip install esptool")
    except subprocess.CalledProcessError as e:
        sys.exit("\nFlash failed (exit %s). If the port didn't respond, enter download mode:\n"
                 "  hold BOOT, tap RESET, release BOOT — then retry." % e.returncode)
    print("\nDone. The device reboots into the new firmware.")


if __name__ == "__main__":
    main()
