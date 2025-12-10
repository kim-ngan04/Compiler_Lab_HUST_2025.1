#!/bin/bash

make

for f in ../test/example*.kpl; do
    base=$(basename "$f" .kpl)
    num=${base#example}

    echo "== Testing $base =="

    ./parser.exe "$f" > "../test/output/${base}.out"

    if diff -u --strip-trailing-cr "../test/result${num}.txt" "../test/output/${base}.out" > /dev/null; then
        echo -e "➡️  PASS"
    else
        echo -e "❌ FAIL"
        echo "---- DIFF ----"
        diff -u --strip-trailing-cr "../test/result${num}.txt" "../test/output/${base}.out"
    fi

    echo
done
