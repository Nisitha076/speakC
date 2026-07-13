#!/bin/bash

SPEAKC="./build/speakc"
PASS=0
FAIL=0
ERRORS=""

echo "=== SpeakC Round-Trip Tests ==="
echo ""

for test_file in tests/samples/roundtrip_*.speakc; do
    base=$(basename "$test_file" .speakc)

    # Step 1: Forward — .speakc → .c
    $SPEAKC build "$test_file" 2>/dev/null
    c_file="tests/samples/${base}.c"

    if [ ! -f "$c_file" ]; then
        echo "FAIL  $base (forward transpilation failed)"
        FAIL=$((FAIL + 1))
        ERRORS="$ERRORS\n  - $base: forward transpilation failed"
        continue
    fi

    # Step 2: Backward — .c → .speakc (save as .roundtrip.speakc)
    # The reverse command overwrites the original .speakc, so we need to
    # rename/copy files to avoid losing the original test files.
    
    # Let's save a copy of the original .speakc first
    cp "$test_file" "tests/samples/${base}.orig.speakc"
    
    # Run reverse which overwrites the test_file (.speakc)
    $SPEAKC reverse "$c_file" 2>/dev/null
    
    # Save the reversed .speakc as .roundtrip.speakc
    mv "$test_file" "tests/samples/${base}.roundtrip.speakc"
    
    # Restore the original .speakc
    mv "tests/samples/${base}.orig.speakc" "$test_file"
    
    # Save the original generated C file
    cp "$c_file" "tests/samples/${base}.roundtrip1.c"

    # Step 3: Forward again — roundtrip .speakc → .c
    $SPEAKC build "tests/samples/${base}.roundtrip.speakc" 2>/dev/null
    roundtrip_c="tests/samples/${base}.c"
    cp "$roundtrip_c" "tests/samples/${base}.roundtrip2.c"

    # Step 4: Compare the two C files
    if diff -q "tests/samples/${base}.roundtrip1.c" "tests/samples/${base}.roundtrip2.c" > /dev/null 2>&1; then
        echo "PASS  $base"
        PASS=$((PASS + 1))
    else
        echo "FAIL  $base (round-trip mismatch)"
        echo "  Diff:"
        diff "tests/samples/${base}.roundtrip1.c" "tests/samples/${base}.roundtrip2.c" | head -20
        FAIL=$((FAIL + 1))
        ERRORS="$ERRORS\n  - $base: C output differs after round-trip"
    fi

    # Cleanup temporary files
    rm -f "tests/samples/${base}.roundtrip1.c" "tests/samples/${base}.roundtrip2.c" "tests/samples/${base}.roundtrip.speakc" "$c_file"
done

echo ""
echo "=== Results ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"

if [ $FAIL -gt 0 ]; then
    echo ""
    echo "Failures:"
    echo -e "$ERRORS"
    exit 1
fi

echo ""
echo "All round-trip tests passed! 🎉"
exit 0
