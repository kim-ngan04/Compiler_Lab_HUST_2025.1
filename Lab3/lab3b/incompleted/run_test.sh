#!/bin/bash

make

# Tạo thư mục output nếu chưa có
mkdir -p ../tests/output

for f in ../tests/example*.kpl; do
    if [ ! -f "$f" ]; then
        continue
    fi
    
    base=$(basename "$f" .kpl)
    num=${base#example}

    echo "== Testing $base =="

    ./kplc "$f" > "../tests/output/${base}.out" 2>&1

    result_file="../tests/result${num}.txt"
    output_file="../tests/output/${base}.out"
    
    if [ ! -f "$result_file" ]; then
        echo "❌ FAIL - Result file not found: $result_file"
    elif diff -u --strip-trailing-cr "$result_file" "$output_file" > /dev/null 2>&1; then
        echo "✅ PASS"
    else
        echo "❌ FAIL"
        echo "---- DIFF ----"
        diff -u --strip-trailing-cr "$result_file" "$output_file"
    fi

    echo
done
