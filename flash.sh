#!/usr/bin/env bash
# Compile a sensor sketch and flash it over USB.
#
#   ./flash.sh humidity-sensor
#   ./flash.sh co2-sensor --check              # compile only, no board attached
#   PORT=/dev/cu.usbmodem1101 ./flash.sh brightness-sensor
#   FQBN=esp32:esp32:esp32c6 ./flash.sh co2-sensor   # skip chip detection
#
# The board is not fixed: the sketches run on whatever ESP32 variant is plugged
# in, so the FQBN is derived from the chip esptool finds on the port (a C6 and a
# classic ESP32 both build the same source, but only the matching FQBN uploads).
#
# Credentials come from common/config.h, which is gitignored; this refuses to
# build without it rather than flashing placeholder values.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$root"

die() { echo "error: $*" >&2; exit 1; }
note() { echo "==> $*"; }

sketch="${1:-}"
check=0
[[ "${2:-}" == "--check" ]] && check=1

[[ -n "$sketch" ]] || die "usage: ./flash.sh <sketch> [--check]"
[[ -f "$sketch/$sketch.ino" ]] || die "$sketch/$sketch.ino not found"
command -v arduino-cli >/dev/null || die "arduino-cli not found (brew install arduino-cli)"

# Only the uploading sketches need credentials; the i2c scanner does not.
if grep -q "common/acta.h" "$sketch/$sketch.ino"; then
    [[ -f common/config.h ]] || die "common/config.h is missing:
  cp common/config.example.h common/config.h   # then fill in the SSID and the acta credential"
fi

# Libraries each sketch pulls in. `arduino-cli lib install` takes the index names,
# which is what the error message prints when one is missing.
case "$sketch" in
    co2-sensor) libs=("SparkFun SCD30 Arduino Library") ;;
    humidity-sensor) libs=("DHT20") ;;
    brightness-sensor) libs=("BH1750" "Adafruit BMP280 Library") ;;
    *) libs=() ;;
esac

if ((${#libs[@]})); then
    installed="$(arduino-cli lib list 2>/dev/null || true)"
    missing=()
    for lib in "${libs[@]}"; do
        grep -qiF "$lib" <<<"$installed" || missing+=("$lib")
    done
    if ((${#missing[@]})); then
        die "missing libraries: ${missing[*]}
  arduino-cli lib install $(printf '"%s" ' "${missing[@]}")"
    fi
fi

# arduino-cli's bundled ctags tool is x86_64-only; without Rosetta every compile
# dies before it reaches the compiler, with an error that says nothing about the
# cause. Say what to do instead of echoing "bad CPU type".
compile() {
    local log
    log="$(mktemp)"
    if ! "$@" 2>&1 | tee "$log"; then
        if grep -qi "bad cpu type" "$log"; then
            cat >&2 <<'EOF'
error: arduino-cli's ctags tool is x86_64-only and Rosetta is not installed, so
  the build stops before compiling. Install Rosetta once (needs sudo):
      softwareupdate --install-rosetta
EOF
        fi
        rm -f "$log"
        exit 1
    fi
    rm -f "$log"
}

# First USB serial port that is not one of macOS's Bluetooth or debug consoles.
detect_port() {
    arduino-cli board list 2>/dev/null |
        awk '$2 == "serial" && $1 ~ /^\/dev\/cu\.(usb|wchusbserial|SLAB_USBtoUART)/ { print $1; exit }'
}

# esptool ships with the esp32 core, not on PATH.
esptool() {
    local data_dir tool
    data_dir="$(arduino-cli config get directories.data 2>/dev/null || true)"
    [[ -n "$data_dir" ]] || data_dir="$HOME/Library/Arduino15"
    tool="$(ls -d "$data_dir"/packages/esp32/tools/esptool_py/*/ 2>/dev/null | sort -V | tail -1)"
    [[ -n "$tool" ]] || return 1
    "${tool}esptool" "$@"
}

# Generic dev-module FQBN for the chip on the port. Ordered longest-first: plain
# `ESP32` is a prefix of every other variant.
fqbn_for_chip() {
    case "$1" in
        ESP32-C6*) echo "esp32:esp32:esp32c6" ;;
        ESP32-C5*) echo "esp32:esp32:esp32c5" ;;
        ESP32-C3*) echo "esp32:esp32:esp32c3" ;;
        ESP32-C2*) echo "esp32:esp32:esp32c2" ;;
        ESP32-S3*) echo "esp32:esp32:esp32s3" ;;
        ESP32-S2*) echo "esp32:esp32:esp32s2" ;;
        ESP32-H2*) echo "esp32:esp32:esp32h2" ;;
        ESP32-P4*) echo "esp32:esp32:esp32p4" ;;
        ESP32*) echo "esp32:esp32:esp32" ;;
        *) return 1 ;;
    esac
}

detect_chip() {
    local out
    out="$(esptool --port "$1" flash-id 2>&1 || true)"
    sed -n 's/^Chip type: *\(ESP32[^ ]*\).*/\1/p' <<<"$out" | head -1
}

port="${PORT:-}"
[[ -n "$port" ]] || port="$(detect_port || true)"

fqbn="${FQBN:-}"
if [[ -z "$fqbn" && -f "$sketch/fqbn" ]]; then
    # A board that detection cannot tell apart from a generic dev module: the
    # wiring lives in the board definition (XIAO C6 swaps SDA/SCL, for example),
    # so the FQBN is part of the device, recorded next to its sketch.
    fqbn="$(tr -d '[:space:]' < "$sketch/fqbn")"
fi
if [[ -z "$fqbn" && -n "$port" ]]; then
    chip="$(detect_chip "$port" || true)"
    [[ -n "$chip" ]] && fqbn="$(fqbn_for_chip "$chip" || true)"
fi
if [[ -z "$fqbn" ]]; then
    fqbn="esp32:esp32:esp32"
    note "no board on a USB port; building for the generic ${fqbn} (set FQBN=… to pick another)"
fi

# A board reached over its native USB port needs `Serial` bound to USB CDC,
# otherwise the monitor sees nothing. Only boards that expose the option get it,
# and only when no options were given explicitly (three colon-separated fields).
if [[ "$port" == *usbmodem* && "$fqbn" != *:*:*:* ]] &&
    arduino-cli board details -b "$fqbn" 2>/dev/null | grep -q "CDCOnBoot="; then
    fqbn="${fqbn}:CDCOnBoot=cdc"
fi

if ((check)); then
    note "compiling ${sketch} for ${fqbn}"
    compile arduino-cli compile --fqbn "$fqbn" "$sketch"
    exit 0
fi

[[ -n "$port" ]] || die "no USB serial port found; plug the device in or set PORT=/dev/cu.usbmodem…"

note "flashing ${sketch} to ${port} (${fqbn})"
compile arduino-cli compile --fqbn "$fqbn" --upload --port "$port" "$sketch"

note "flashed; watch it with: ./monitor.sh ${port}"
note "a reading is live once the log shows \"[acta] <source> accepted\""
