#!/usr/bin/env bash
# Compile a sensor sketch and flash it over USB.
#
#   ./flash.sh humidity-sensor
#   ./flash.sh co2-sensor --check              # compile only, no board attached
#   PORT=/dev/cu.usbserial-1130 ./flash.sh brightness-sensor
#   FQBN=esp32:esp32:lolin_d32 ./flash.sh co2-sensor
#
# The sketches read their credentials from common/config.h, which is gitignored;
# this refuses to build without it rather than flashing placeholder values.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$root"

die() { echo "error: $*" >&2; exit 1; }
note() { echo "==> $*"; }

fqbn="${FQBN:-esp32:esp32:esp32}"
sketch="${1:-}"
check=0
[[ "${2:-}" == "--check" ]] && check=1

[[ -n "$sketch" ]] || die "usage: ./flash.sh <sketch> [--check]"
[[ -f "$sketch/$sketch.ino" ]] || die "$sketch/$sketch.ino not found"
command -v arduino-cli >/dev/null || die "arduino-cli not found (brew install arduino-cli)"

[[ -f common/config.h ]] || die "common/config.h is missing:
  cp common/config.example.h common/config.h   # then fill in the SSID and the acta credential"

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

if ((check)); then
    note "compiling ${sketch} for ${fqbn}"
    compile arduino-cli compile --fqbn "$fqbn" "$sketch"
    exit 0
fi

# First USB serial port that is not one of macOS's Bluetooth or debug consoles.
# `arduino-cli board list` also reports the board, but a device that is awake and
# talking still enumerates as a plain serial port, which is all we need.
port="${PORT:-}"
if [[ -z "$port" ]]; then
    port="$(arduino-cli board list 2>/dev/null |
        awk '$2 == "serial" && $1 ~ /^\/dev\/cu\.(usb|wchusbserial|SLAB_USBtoUART)/ { print $1; exit }')"
fi
[[ -n "$port" ]] || die "no USB serial port found; plug the device in or set PORT=/dev/cu.usbmodem…"

note "flashing ${sketch} to ${port} (${fqbn})"
compile arduino-cli compile --fqbn "$fqbn" --upload --port "$port" "$sketch"

note "flashed; watch it with: ./monitor.sh ${port}"
note "a reading is live once the log shows \"[acta] <source> accepted\""
