#!/bin/bash

# Reports the VRAM CRCs the emulator actually produces, so expected_CRCs in
# autotest.json can be updated after an intended rendering change.
#
# Usage: ./test/capture-hashes.sh

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="$ROOT/autotest.json"
ROM="$ROOT/test/ti84pce.rom"

if [ ! -f "$ROM" ]; then
    echo "No ROM at test/ti84pce.rom - see test/README.md" >&2
    exit 1
fi

# The autotester wants the config path in native form and chdirs to its folder
if command -v cygpath >/dev/null 2>&1; then
    CONFIG="$(cygpath -w "$CONFIG")"
fi

echo "Building before capturing hashes..."
if ! make -C "$ROOT" build; then
    echo "Build failed, not capturing hashes." >&2
    exit 1
fi

echo
echo "Running autotester..."
output="$(cemu-autotester "$CONFIG" 2>&1)"
echo "$output"

echo
mismatches="$(echo "$output" | grep -o 'Hash #[0-9]* .*(got [0-9A-F]*)')"

if [ -z "$mismatches" ]; then
    echo "All hashes matched. Nothing to update."
    exit 0
fi

echo "Update these in autotest.json only if the change was intended:"
echo "$mismatches" | sed -E 's/Hash #([0-9]+).*\(got ([0-9A-F]+)\)/  hash \1 -> "\2"/'
