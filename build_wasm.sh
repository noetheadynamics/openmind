#!/bin/bash
# OpenMind WASM Build Script for Linux/macOS
# Requires Emscripten SDK installed and activated

set -e

echo "========================================"
echo " OpenMind WebAssembly Build"
echo "========================================"

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo "ERROR: Emscripten not found in PATH."
    echo "Please activate Emscripten SDK first:"
    echo "  source ./emsdk_env.sh"
    echo ""
    echo "Or install from: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

echo "[1/4] Cleaning previous build..."
rm -rf build_wasm
mkdir -p build_wasm
cd build_wasm

echo "[2/4] Configuring with CMake + Emscripten..."
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

echo "[3/4] Building WASM module..."
cmake --build . --config Release

echo "[4/4] Copying output files..."
cd ..
cp build_wasm/openmind.js . 2>/dev/null || true
cp build_wasm/openmind.wasm . 2>/dev/null || true

echo ""
echo "========================================"
echo " BUILD SUCCESSFUL"
echo "========================================"
echo ""
echo "Output files:"
[ -f openmind.js ]   && echo "  openmind.js   - JS loader"
[ -f openmind.wasm ] && echo "  openmind.wasm - WASM binary"
echo ""
echo "To test, open index.html in a browser."
echo ""
