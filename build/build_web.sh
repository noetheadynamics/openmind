#!/bin/bash
# build_web.sh — Web build (copies assets to dist/)
set -e

echo "=== OpenMind Web Build ==="

DIST_DIR="build/dist"

mkdir -p "$DIST_DIR"

echo "Copying HTML..."
cp src/ui/omni_console.html "$DIST_DIR/index.html"

echo "Copying CSS..."
cp src/ui/omni_console.css "$DIST_DIR/"

echo "Copying JavaScript..."
cp src/ui/omni_console.js "$DIST_DIR/"
cp src/ui/ui_connection.js "$DIST_DIR/"

echo "Copying WASM output (if exists)..."
if [ -f build/wasm/openmind.js ]; then
    cp build/wasm/openmind.js "$DIST_DIR/"
    cp build/wasm/openmind.wasm "$DIST_DIR/"
fi

echo "Copying demo world..."
cp demo_world.json "$DIST_DIR/" 2>/dev/null || echo "  (demo_world.json not found, skipping)"

echo "Web build complete: $DIST_DIR/"
ls -la "$DIST_DIR"/
