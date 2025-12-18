/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * FILE: main.c
 * MỤC ĐÍCH:
 *   - Chương trình chính của trình biên dịch
 *   - Đọc tên file từ dòng lệnh
 *   - Gọi hàm compile() để biên dịch file
 *   - Xử lý lỗi đầu vào
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Kiểm tra xem có file input hay không
 *   2. Nếu không có → in lỗi và thoát
 *   3. Nếu có → gọi compile(fileName)
 *   4. Nếu compile thành công → in kết quả
 *   5. Nếu có lỗi → in thông báo lỗi
 */

#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "parser.h"

/******************************************************************/

/* 
 * HÀM MAIN: Điểm bắt đầu của chương trình
 * 
 * THAM SỐ:
 *   - argc: số lượng tham số dòng lệnh
 *   - argv: mảng các chuỗi tham số dòng lệnh
 *           argv[0] = tên chương trình
 *           argv[1] = tên file cần biên dịch
 * 
 * TRẢ VỀ:
 *   - 0: nếu chạy thành công
 *   - -1: nếu có lỗi
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Kiểm tra argc ≤ 1 → không có file input
 *   2. Gọi compile(argv[1]) → biên dịch file
 *   3. Nếu trả về IO_ERROR → file không thể đọc
 *   4. Nếu compile thành công → chương trình sẽ in kết quả
 */
int main(int argc, char *argv[]) {
  // Kiểm tra xem có đủ tham số không (tên file)
  if (argc <= 1) {
    printf("parser: no input file.\n");
    return -1;
  }

  // Gọi hàm biên dịch với tên file từ argv[1]
  if (compile(argv[1]) == IO_ERROR) {
    printf("Can\'t read input file!\n");
    return -1;
  }
    
  return 0;
}
