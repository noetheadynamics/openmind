#!/bin/bash
# build_wasm.sh — Emscripten build for OpenMind
# Requires: emsdk (https://emscripten.org/docs/getting_started/downloads.html)
set -e

echo "=== OpenMind WASM Build ==="

if ! command -v emcc &> /dev/null; then
    echo "Error: emcc not found. Install emsdk first:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

SRC_DIR="src/engine"
LLM_DIR="src/llm"
AGENT_DIR="src/agents"
BRIDGE_DIR="src/bridge"
OUT_DIR="build/wasm"
OUT_FILE="openmind.js"

mkdir -p "$OUT_DIR"

echo "Compiling engine..."
emcc -O3 -std=c++17 \
    "$SRC_DIR/VoxelOctree.cpp" \
    "$SRC_DIR/PhysicsEngine.cpp" \
    "$SRC_DIR/openmind_engine.cpp" \
    "$LLM_DIR/HttpClient.cpp" \
    "$LLM_DIR/OpenAIClient.cpp" \
    "$LLM_DIR/AnthropicClient.cpp" \
    "$LLM_DIR/GoogleClient.cpp" \
    "$LLM_DIR/OllamaClient.cpp" \
    "$LLM_DIR/OpenAICompatibleClient.cpp" \
    "$LLM_DIR/JSONValidator.cpp" \
    "$LLM_DIR/AsyncRequestManager.cpp" \
    "$BRIDGE_DIR/openmind_bridge.cpp" \
    -I"$SRC_DIR" -I"$LLM_DIR" -I"$AGENT_DIR" -I"$BRIDGE_DIR" \
    -s EXPORTED_FUNCTIONS="['_initWorld','_setBlock','_setBlockSimple','_getBlock','_removeBlock','_tickPhysicsDelta','_getWorldStats','_setTimeOfDay','_getTimeOfDay','_getSunlightIntensity','_setCycleDuration','_rewindTime','_setWeather','_getWeather','_setTimeScale','_getAgentCount','_getAgentData','_setAgentPosition','_teleportCamera','_setOverlay','_generateFromPrompt','_exportWorld','_saveWorld']" \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=67108864 \
    -s WASM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="OpenMindModule" \
    -s ENVIRONMENT='web' \
    -s SINGLE_FILE=0 \
    -o "$OUT_DIR/$OUT_FILE"

echo "Build complete: $OUT_DIR/$OUT_FILE"
echo "Files:"
ls -la "$OUT_DIR"/openmind.*
