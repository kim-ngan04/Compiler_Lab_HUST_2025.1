#!/bin/bash

set -o pipefail

echo "== BUILD =="
if ! make; then
    echo "❌ Build failed"
    exit 1
fi
echo

mkdir -p ../tests/output

all_pass=true

for f in ../tests/example*.kpl; do
    [ -f "$f" ] || continue

    base=$(basename "$f" .kpl)
    num=${base#example}

    echo "== Testing $base =="

    output_file="../tests/output/${base}.out"
    result_file="../tests/result${num}.txt"

    ./kplc "$f" > "$output_file" 2>&1
    status=$?

    if [ $status -ne 0 ]; then
        echo "❌ FAIL - Runtime error (exit code $status)"
        cat "$output_file"
        all_pass=false
        echo
        continue
    fi

    if [ ! -f "$result_file" ]; then
        echo "❌ FAIL - Missing result file: $result_file"
        all_pass=false
        echo
        continue
    fi

    if diff -u --strip-trailing-cr "$result_file" "$output_file" > /dev/null; then
        echo "✅ PASS"
    else
        echo "❌ FAIL"
        echo "---- DIFF ----"
        diff -u --strip-trailing-cr "$result_file" "$output_file"
        all_pass=false
    fi

    echo
done

if $all_pass; then
    echo "🎉 ALL TESTS PASSED"
else
    echo "❌ SOME TESTS FAILED"
    exit 1
fi
