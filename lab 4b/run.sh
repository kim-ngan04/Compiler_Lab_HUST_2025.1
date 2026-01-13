#!/bin/bash

set +e

TEST_DIR="tests"
TMP_DIR="tests/_tmp_output"

mkdir -p "$TMP_DIR"

PASS=0
FAIL=0
TOTAL=0

echo "========================================"
echo "Running KPL tests using kplc.exe"
echo "========================================"

for SRC in "$TEST_DIR"/*.kpl; do
  TOTAL=$((TOTAL + 1))

  NAME=$(basename "$SRC" .kpl)
  EXP_BIN="$TEST_DIR/$NAME"
  OUT_BIN="$TMP_DIR/$NAME"

  echo "----------------------------------------"
  echo "Testing $NAME.kpl"

  # -------------------------------
  # Check expected binary
  # -------------------------------
  if [ ! -f "$EXP_BIN" ]; then
    echo "❌ Expected binary not found: $EXP_BIN"
    FAIL=$((FAIL + 1))
    continue
  fi

  # -------------------------------
  # Remove old output
  # -------------------------------
  rm -f "$OUT_BIN"

  # -------------------------------
  # Run kplc.exe (CORRECT USAGE)
  # -------------------------------
  ./kplc.exe "$SRC" "$OUT_BIN" -dump
  if [ $? -ne 0 ]; then
    echo "❌ kplc.exe failed"
    FAIL=$((FAIL + 1))
    continue
  fi

  # -------------------------------
  # Check generated binary
  # -------------------------------
  if [ ! -f "$OUT_BIN" ]; then
    echo "❌ Output binary not generated"
    FAIL=$((FAIL + 1))
    continue
  fi

  # -------------------------------
  # Compare file size
  # -------------------------------
  SIZE_OUT=$(stat -c%s "$OUT_BIN")
  SIZE_EXP=$(stat -c%s "$EXP_BIN")

  if [ "$SIZE_OUT" -ne "$SIZE_EXP" ]; then
    echo "❌ FAIL (Size mismatch)"
    echo "   Output   : $SIZE_OUT bytes"
    echo "   Expected : $SIZE_EXP bytes"
    FAIL=$((FAIL + 1))
    continue
  fi

  # -------------------------------
  # Compare binary content
  # -------------------------------
  if cmp -s "$OUT_BIN" "$EXP_BIN"; then
    echo "✅ PASS"
    PASS=$((PASS + 1))
  else
    echo "❌ FAIL (Binary content differs)"
    FAIL=$((FAIL + 1))
  fi
done

echo "========================================"
echo "Total : $TOTAL"
echo "PASS  : $PASS"
echo "FAIL  : $FAIL"
echo "========================================"
