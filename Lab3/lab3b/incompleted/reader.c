/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * FILE: reader.c
 * MỤC ĐÍCH:
 *   - Quản lý việc đọc ký tự từ file input
 *   - Theo dõi vị trí dòng và cột hiện tại
 *   - Cung cấp các hàm để mở/đóng file
 * 
 * CÁC BIẾN GLOBAL:
 *   - inputStream: con trỏ FILE để đọc input
 *   - lineNo: số dòng hiện tại
 *   - colNo: số cột hiện tại
 *   - currentChar: ký tự hiện tại đang xử lý
 */

#include <stdio.h>
#include "reader.h"

FILE *inputStream;  /* Con trỏ FILE của file input */
int lineNo, colNo;  /* Vị trí dòng và cột hiện tại */
int currentChar;    /* Ký tự hiện tại */

/* 
 * HÀM readChar: Đọc ký tự tiếp theo từ file
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Đọc một ký tự từ inputStream bằng getc()
 *   2. Tăng colNo (cột) lên 1
 *   3. Nếu ký tự là '\n' (xuống dòng):
 *      - Tăng lineNo lên 1
 *      - Đặt colNo = 0 (cột mới)
 *   4. Trả về ký tự đã đọc
 * 
 * TRẢ VỀ:
 *   - Ký tự được đọc (hoặc EOF nếu hết file)
 * 
 * VÍ DỤ:
 *   readChar();  // Đọc ký tự đầu tiên
 *   // currentChar = 'p', lineNo = 1, colNo = 1
 *   
 *   readChar();  // Đọc '\n'
 *   // currentChar = '\n', lineNo = 2, colNo = 0
 */
int readChar(void) {
  currentChar = getc(inputStream);  // Đọc ký tự từ file
  colNo++;  // Tăng cột
  if (currentChar == '\n') {
    lineNo++;  // Xuống dòng → tăng dòng
    colNo = 0;  // Đặt cột về 0
  }
  return currentChar;
}

/* 
 * HÀM openInputStream: Mở file input
 * 
 * THAM SỐ:
 *   - fileName: tên của file cần mở
 * 
 * TRẢ VỀ:
 *   - IO_SUCCESS: nếu mở file thành công
 *   - IO_ERROR: nếu mở file thất bại
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Mở file fileName ở chế độ text ("rt")
 *   2. Nếu fopen() trả về NULL → lỗi, trả về IO_ERROR
 *   3. Nếu thành công:
 *      - Khởi tạo lineNo = 1, colNo = 0
 *      - Gọi readChar() để đọc ký tự đầu tiên
 *      - Trả về IO_SUCCESS
 * 
 * VÍ DỤ:
 *   if (openInputStream("program.kpl") == IO_SUCCESS) {
 *       // File được mở thành công
 *       // currentChar chứa ký tự đầu tiên
 *   }
 */
int openInputStream(char *fileName) {
  inputStream = fopen(fileName, "rt");  // Mở file ở chế độ text
  if (inputStream == NULL)
    return IO_ERROR;  // Lỗi mở file
  lineNo = 1;  // Bắt đầu từ dòng 1
  colNo = 0;   // Bắt đầu từ cột 0
  readChar();  // Đọc ký tự đầu tiên
  return IO_SUCCESS;
}

/* 
 * HÀM closeInputStream: Đóng file input
 * 
 * CÁCH HOẠT ĐỘNG:
 *   - Đóng file inputStream bằng fclose()
 *   - Giải phóng tài nguyên
 * 
 * VÍ DỤ:
 *   closeInputStream();  // Đóng file
 */
void closeInputStream() {
  fclose(inputStream);  // Đóng file
}

