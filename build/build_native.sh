#!/bin/bash
# build_native.sh — Native build for testing (Linux/macOS)
set -e

echo "=== OpenMind Native Build ==="

SRC_DIR="src/engine"
LLM_DIR="src/llm"
AGENT_DIR="src/agents"
TEST_DIR="tests"
OUT_DIR="build/native"

mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"

compile_test() {
    local test=$1
    local name=$(basename "$test" .cpp)
    echo "  Compiling $name..."
    $CXX $CXXFLAGS \
        "$test" \
        "$SRC_DIR/VoxelOctree.cpp" \
        "$SRC_DIR/PhysicsEngine.cpp" \
        "$SRC_DIR/openmind_engine.cpp" \
        "$LLM_DIR/JSONValidator.cpp" \
        -I"$SRC_DIR" -I"$LLM_DIR" -I"$AGENT_DIR" \
        -o "$OUT_DIR/$name" \
        -lpthread 2>/dev/null || true
}

echo "Compiling tests..."
for test in "$TEST_DIR"/test_*.cpp; do
    compile_test "$test"
done

echo "Build complete. Binaries in $OUT_DIR/"
ls "$OUT_DIR"/test_* 2>/dev/null | wc -l | xargs -I{} echo "{} tests compiled"
