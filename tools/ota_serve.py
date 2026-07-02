#!/usr/bin/env python3
"""ota_serve.py — host a firmware update on your LAN so the device updates from you.

After the first USB flash, updates go over the air: the device's OTA (Settings →
Check update) downloads the app image from its `fw_url`. Run this on your machine,
point `fw_url` at the URL it prints, and the device self-updates over Wi-Fi — nothing
in the cloud, you host nothing permanently.

    python tools/ota_serve.py                    # serve ./build/<app>.bin
    python tools/ota_serve.py --bin build/app.bin --port 8000

The device and this machine must be on the same network. Set `fw_url` via the device's
web-config page (Settings → Web config), then trigger Settings → Check update.

NOTE: this serves over plain HTTP for LAN use — the device's OTA must allow an http://
`fw_url` (the cloud/HTTPS path uses the cert bundle instead).
"""
import argparse
import functools
import glob
import http.server
import os
import socket
import sys

# Files under build/ that are NOT the app image (so we can auto-pick the app .bin).
_NOT_APP = ("bootloader.bin", "partition-table.bin", "partition_table.bin",
            "ota_data_initial.bin")


def lan_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


def find_app_bin(build_dir):
    cands = [f for f in glob.glob(os.path.join(build_dir, "*.bin"))
             if os.path.basename(f) not in _NOT_APP]
    return max(cands, key=os.path.getsize) if cands else None


def main():
    ap = argparse.ArgumentParser(description="Serve a firmware image for LAN OTA.")
    ap.add_argument("--bin", help="the app image to serve (default: ./build/<app>.bin)")
    ap.add_argument("--build-dir", default="build")
    ap.add_argument("--port", type=int, default=8000)
    args = ap.parse_args()

    binpath = args.bin or find_app_bin(args.build_dir)
    if not binpath or not os.path.exists(binpath):
        sys.exit("No firmware image found. Run `idf.py build` first, or pass --bin <app.bin>.")

    binpath = os.path.abspath(binpath)
    serve_dir = os.path.dirname(binpath)
    name = os.path.basename(binpath)
    url = "http://%s:%d/%s" % (lan_ip(), args.port, name)

    print("Serving %s" % binpath)
    print("\n  Point the device's fw_url at:\n      %s\n" % url)
    print("  (Settings -> Web config -> fw_url), then Settings -> Check update.")
    print("Ctrl-C to stop.\n")

    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=serve_dir)
    try:
        http.server.HTTPServer(("0.0.0.0", args.port), handler).serve_forever()
    except KeyboardInterrupt:
        print("\nstopped.")


if __name__ == "__main__":
    main()
