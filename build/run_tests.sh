#!/bin/bash
# run_tests.sh — Run all OpenMind test suites
set -e

OUT_DIR="build/native"
PASS=0
FAIL=0
TOTAL=0

echo "=== OpenMind Test Suite ==="
echo ""

run_test() {
    local test=$1
    local name=$(basename "$test")
    TOTAL=$((TOTAL + 1))
    printf "  %-35s" "$name"
    if [ -f "$OUT_DIR/$name" ]; then
        if "$OUT_DIR/$name" > /dev/null 2>&1; then
            echo "PASS"
            PASS=$((PASS + 1))
        else
            echo "FAIL"
            FAIL=$((FAIL + 1))
        fi
    else
        echo "SKIP (not built)"
    fi
}

echo "Running engine tests..."
for test in test_lighting test_timeday test_sound test_reverb test_orbital test_atmosphere test_rocket test_weather test_temperature test_explosion test_acid test_corrosion test_metabolism test_disease test_predator_prey test_plant test_decay; do
    run_test "$test"
done

echo ""
echo "Running LLM tests..."
run_test test_llm

echo ""
echo "Running agent tests..."
run_test test_agent

echo ""
echo "Running wrapper tests..."
run_test test_engine_wrapper

echo ""
echo "========================================="
echo " Results: $PASS passed, $FAIL failed, $TOTAL total"
echo "========================================="

[ $FAIL -eq 0 ] && exit 0 || exit 1
