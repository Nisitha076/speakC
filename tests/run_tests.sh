#!/bin/bash

SPEAKC=./build/speakc
PASS=0
FAIL=0
TOTAL=0

echo "=== SpeakC Test Suite ==="
echo ""

for test_file in tests/samples/*.speakc; do
    TOTAL=$((TOTAL + 1))
    base=$(basename "$test_file" .speakc)
    c_file="tests/samples/${base}.c"
    bin_file="tests/samples/${base}"

    # Step 1: Transpile
    $SPEAKC build "$test_file" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "FAIL  $base  (transpile error)"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Step 2: Compile with gcc
    gcc -std=c17 -Wall -o "$bin_file" "$c_file" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "FAIL  $base  (gcc compile error)"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Step 3: Run and check it doesn't crash
    output=$("$bin_file" 2>&1)
    if [ $? -ne 0 ]; then
        echo "FAIL  $base  (runtime error)"
        FAIL=$((FAIL + 1))
        continue
    fi

    echo "PASS  $base"
    PASS=$((PASS + 1))

    # Cleanup binary and generated C file
    rm -f "$bin_file" "$c_file"
done

echo ""
echo "=== Results: $PASS/$TOTAL passed, $FAIL failed ==="

if [ $FAIL -gt 0 ]; then
    exit 1
fi