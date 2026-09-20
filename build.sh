#!/bin/bash

# Build script for DoomNanoCE port to TI-84+CE
#
# Usage: ./build.sh [--no-test]
#
# Builds the project and then runs the CEmu autotests, unless --no-test is
# given or no calculator ROM has been supplied (see test/README.md).

RUN_TESTS=1
if [ "${1:-}" = "--no-test" ]; then
    RUN_TESTS=0
fi

echo "Building DoomNanoCE for TI-84+CE..."

# Clean previous builds
echo "Cleaning previous build artifacts..."
make clean

# Build the project
echo "Building project..."
if make; then
    echo "Build completed successfully!"
    echo "Output file: bin/DOOM.8xp"
else
    echo "Build failed!"
    exit 1
fi

# Run the emulator tests against the binary we just produced
if [ "$RUN_TESTS" -eq 0 ]; then
    echo "Skipping tests (--no-test)."
elif [ ! -f test/ti84pce.rom ]; then
    echo "Skipping tests: no ROM at test/ti84pce.rom (see test/README.md)."
else
    echo "Running CEmu autotests..."
    if make test; then
        echo "Tests passed!"
    else
        echo "Tests failed!"
        exit 1
    fi
fi

echo "Build process finished."
