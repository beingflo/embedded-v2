#!/usr/bin/env bash
# Watch a sensor's serial output.
#
#   ./monitor.sh                            # auto-detect the port, 921600 baud
#   ./monitor.sh /dev/cu.usbmodem1101
#   BAUD=115200 ./monitor.sh                # the old ESP-NOW sketches
#
# The uploading sketches print their reading, then "[acta] <source> accepted" or
# the rejection body from the service, which is what tells a 401 (wrong
# credential) from a 400 (bad row).
set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }

command -v arduino-cli >/dev/null || die "arduino-cli not found (brew install arduino-cli)"

port="${1:-}"
if [[ -z "$port" ]]; then
    port="$(arduino-cli board list 2>/dev/null |
        awk '$2 == "serial" && $1 ~ /^\/dev\/cu\.(usb|wchusbserial|SLAB_USBtoUART)/ { print $1; exit }')"
fi
[[ -n "$port" ]] || die "no USB serial port found; pass one, e.g. ./monitor.sh /dev/cu.usbmodem1101"

exec arduino-cli monitor -p "$port" --config "baudrate=${BAUD:-921600}"
