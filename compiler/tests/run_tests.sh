#!/bin/bash
# H++ compiler test runner

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/../.."
HPP="$ROOT/build/hpp"
PASS=0
FAIL=0
TOTAL=0

run_test() {
    local name="$1"
    local source="$2"
    local expected_exit="$3"

    TOTAL=$((TOTAL + 1))

    $HPP "$source" -o /tmp/hpp_test_bin 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL: $name (compilation failed)"
        FAIL=$((FAIL + 1))
        return
    fi

    /tmp/hpp_test_bin > /dev/null 2>&1
    actual_exit=$?

    if [ "$actual_exit" -eq "$expected_exit" ]; then
        echo "  PASS: $name (exit=$actual_exit)"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $name (expected exit=$expected_exit, got exit=$actual_exit)"
        FAIL=$((FAIL + 1))
    fi
}

run_test_output() {
    local name="$1"
    local source="$2"
    local expected_output="$3"

    TOTAL=$((TOTAL + 1))

    $HPP "$source" -o /tmp/hpp_test_bin 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL: $name (compilation failed)"
        FAIL=$((FAIL + 1))
        return
    fi

    actual_output=$(/tmp/hpp_test_bin 2>/dev/null)

    if [ "$actual_output" = "$expected_output" ]; then
        echo "  PASS: $name"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $name (output mismatch)"
        echo "    expected: '$expected_output'"
        echo "    got:      '$actual_output'"
        FAIL=$((FAIL + 1))
    fi
}

echo "Running H++ compiler tests..."
echo ""
echo "--- Core language ---"
run_test "return_42"      "$ROOT/examples/return42.hpp"     42
run_test "fibonacci"      "$ROOT/examples/fibonacci.hpp"    55
run_test "arithmetic"     "$ROOT/examples/arithmetic.hpp"   54
run_test "if_else"        "$ROOT/examples/ifelse.hpp"       42
run_test "loops"          "$ROOT/examples/loops.hpp"        55
run_test "bitwise"        "$ROOT/examples/bitwise.hpp"      1
run_test "inline_asm"     "$ROOT/examples/hello.hpp"        0

echo ""
echo "--- Standard library ---"
run_test "stdlib_demo"    "$ROOT/examples/stdlib_demo.hpp"    0
run_test "memory_demo"    "$ROOT/examples/memory_demo.hpp"    0
run_test "file_io_demo"   "$ROOT/examples/file_io_demo.hpp"   0
run_test_output "sleep"   "$ROOT/examples/sleep_demo.hpp"     "$(printf '1\n2\n3')"

echo ""
echo "Results: $PASS passed, $FAIL failed, $TOTAL total"

if [ $FAIL -gt 0 ]; then
    exit 1
fi
exit 0
