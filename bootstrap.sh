#!/bin/sh
# arena bootstrap — run once before first build
# Fetches nob.h, then compiles the nob build tool.
# After this: ./nob

set -e

NOB_TAG="v3.10.0"

if ! command -v cc >/dev/null 2>&1; then
    echo "arena: cc not found (macOS: xcode-select --install)"
    exit 1
fi

mkdir -p vendor

echo "arena: fetching nob.h ${NOB_TAG}..."
curl -sfL "https://raw.githubusercontent.com/tsoding/nob.h/${NOB_TAG}/nob.h" \
     -o vendor/nob.h

echo "arena: building nob..."
cc -o nob nob.c

echo "Bootstrap complete. Run ./nob to build and run the tests."
