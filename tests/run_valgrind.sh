#!/bin/bash

SPEAKC="./build/speakc"
PASS=0
FAIL=0

echo "=== Valgrind Memory Tests ==="
echo ""

for test_file in tests/samples/roundtrip_*.speakc; do
    base=$(basename "$test_file" .speakc)

    # Test forward transpilation
    output=$(valgrind --error-exitcode=1 --leak-check=full $SPEAKC build "$test_file" 2>&1)
    if echo "$output" | grep -q "no leaks are possible"; then
        echo "PASS  $base (build - no leaks)"
        PASS=$((PASS + 1))
    else
        echo "FAIL  $base (build - memory issues)"
        echo "$output" | grep -E "ERROR SUMMARY|LEAK SUMMARY|definitely lost"
        FAIL=$((FAIL + 1))
    fi

    # Test reverse transpilation
    c_file="tests/samples/${base}.c"
    # Ensure C file exists for reverse test (build generates it)
    $SPEAKC build "$test_file" > /dev/null 2>&1
    
    if [ -f "$c_file" ]; then
        output=$(valgrind --error-exitcode=1 --leak-check=full $SPEAKC reverse "$c_file" 2>&1)
        if echo "$output" | grep -q "no leaks are possible"; then
            echo "PASS  $base (reverse - no leaks)"
            PASS=$((PASS + 1))
        else
            echo "FAIL  $base (reverse - memory issues)"
            echo "$output" | grep -E "ERROR SUMMARY|LEAK SUMMARY|definitely lost"
            FAIL=$((FAIL + 1))
        fi
        # clean up C file
        rm -f "$c_file"
    else
        echo "FAIL  $base (reverse - skipped because C file missing)"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "=== Results ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
