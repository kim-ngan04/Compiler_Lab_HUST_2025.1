/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include "reader.h"

/*
 * inputStream: Con trỏ FILE dùng để đọc file đầu vào.
 * lineNo:     Biến đếm số dòng hiện tại trong file (bắt đầu từ 1).
 * colNo:      Biến đếm số cột hiện tại trong dòng (bắt đầu từ 0).
 * currentChar: Lưu ký tự vừa đọc được từ file (trả về cho lexer).
 */
FILE *inputStream;
int lineNo, colNo;
int currentChar;

/*
 * readChar():
 *  - Đọc 1 ký tự từ inputStream bằng hàm getc().
 *  - Tăng colNo (vì đã đọc thêm 1 ký tự).
 *  - Nếu ký tự đọc được là '\n' thì:
 *        + Tăng lineNo (sang dòng mới)
 *        + Reset colNo về 0
 *
 * Trả về:
 *  - Ký tự vừa đọc (dạng int), hoặc EOF nếu đã đọc hết file.
 */
int readChar(void) {
  currentChar = getc(inputStream);   // Đọc ký tự kế tiếp từ file
  colNo ++;                          // Tăng số cột

  if (currentChar == '\n') {         // Nếu ký tự là xuống dòng
    lineNo ++;                       // Sang dòng mới
    colNo = 0;                       // Reset lại số cột
  }

  return currentChar;                // Trả về ký tự vừa đọc
}

/*
 * openInputStream(fileName):
 *  - Mở file đầu vào ở chế độ text ("rt").
 *  - Nếu mở thất bại → trả về IO_ERROR.
 *  - Nếu mở thành công:
 *        + Khởi tạo lineNo = 1 (dòng đầu tiên)
 *        + Khởi tạo colNo  = 0 (chưa đọc ký tự nào)
 *        + Gọi readChar() để đọc ký tự đầu tiên
 *
 * Trả về:
 *  - IO_SUCCESS nếu mở file OK.
 *  - IO_ERROR nếu không mở được file.
 */
int openInputStream(char *fileName) {
  inputStream = fopen(fileName, "rt");   // Mở file
  if (inputStream == NULL)               // Mở thất bại
    return IO_ERROR;

  lineNo = 1;     // Bắt đầu ở dòng 1
  colNo = 0;      // Chưa có ký tự nào trong dòng
  readChar();     // Đọc ký tự đầu tiên và cập nhật currentChar

  return IO_SUCCESS;
}

/*
 * closeInputStream():
 *  - Đóng file input khi đã xử lý xong.
 */
void closeInputStream() {
  fclose(inputStream);   // Đóng file
}