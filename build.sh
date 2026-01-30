#!/bin/bash
set -e

# simple build script for the project
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# clean old build
if [[ "$1" == "clean" ]]; then
    rm -rf "$SCRIPT_DIR/build"
    echo "Build directory cleaned"
    exit 0
fi

# create build dir
mkdir -p "$SCRIPT_DIR/build"
cd "$SCRIPT_DIR/build"

# configure and build with clean environment
env -i PATH="$PATH" HOME="$HOME" cmake .. && make

echo ""
echo "Build success! Run with:"
echo "  $SCRIPT_DIR/build/benchmarks/telemetry_bench"
echo "  $SCRIPT_DIR/build/src/sanity_check"
