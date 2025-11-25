#!/bin/bash

# Thư mục chương trình đã biên dịch
INCOMPLETE_DIR="../incompleted"
PARSER="$INCOMPLETE_DIR/parser.exe"

# Thư mục chứa test và output
TEST_DIR="."
OUTPUT_DIR="$TEST_DIR/output"

# Tạo folder output nếu chưa tồn tại
if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
    echo "Created output directory: $OUTPUT_DIR"
fi

echo "Running tests..."

# Duyệt tất cả file *.kpl
for input_file in "$TEST_DIR"/*.kpl; do
    filename=$(basename "$input_file" .kpl)
    output_file="$OUTPUT_DIR/${filename}.txt"
    expected_file="$TEST_DIR/result${filename##*example}.txt"

    # Chạy parser.exe với output được chỉ định
    "$PARSER" "$input_file" "$output_file"

    # Kiểm tra xem output có được tạo không
    if [ ! -f "$output_file" ]; then
        echo "$filename: FAIL (No output generated)"
        continue
    fi

    # Kiểm tra kết quả
    if diff -q "$output_file" "$expected_file" >/dev/null 2>&1; then
        echo "$filename: PASS"
    else
        echo "$filename: FAIL"
        echo "  Differences:"
        diff "$output_file" "$expected_file"
    fi
done

echo "All tests finished."
