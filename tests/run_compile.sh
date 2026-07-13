#!/bin/bash

SPEAKC="./build/speakc"
PASS=0
FAIL=0

echo "=== SpeakC Compilation Tests ==="
echo ""

for test_file in tests/samples/roundtrip_*.speakc; do
    base=$(basename "$test_file" .speakc)

    # Transpile to C
    $SPEAKC build "$test_file" 2>/dev/null
    c_file="tests/samples/${base}.c"

    if [ ! -f "$c_file" ]; then
        echo "FAIL  $base (transpilation failed)"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Compile with gcc
    gcc -std=c17 -Wall -Wextra -pedantic -o "tests/samples/${base}" "$c_file" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "PASS  $base (compiles with gcc)"
        PASS=$((PASS + 1))
    else
        echo "FAIL  $base (gcc compilation error)"
        gcc -std=c17 -Wall -Wextra -pedantic -o "tests/samples/${base}" "$c_file"
        FAIL=$((FAIL + 1))
    fi

    # Cleanup
    rm -f "tests/samples/${base}" "$c_file"
done

echo ""
echo "=== Results ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
