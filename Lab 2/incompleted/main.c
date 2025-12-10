/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "reader.h"     // Chứa hàm mở file, đọc ký tự cho scanner
#include "parser.h"     // Chứa hàm compile(), parseProgram(),... phụ trách phân tích cú pháp

/******************************************************************/

/*
 * Hàm main(...)
 * --------------------------------------------------------------
 * Đây là điểm vào của chương trình parser.
 * Khi chạy: parser <input_file>
 *
 * Chức năng chính:
 *   1. Kiểm tra xem người dùng có truyền tên file nguồn hay không.
 *   2. Gọi hàm compile(...) để thực hiện:
 *        - mở file
 *        - đọc / quét token (scanner)
 *        - phân tích cú pháp (parser)
 *   3. Nếu file không mở được → báo lỗi IO_ERROR.
 */
int main(int argc, char *argv[]) {
  
  // argc <= 1 nghĩa là chỉ chạy: parser (không truyền filename)
  if (argc <= 1) {
    printf("parser: no input file.\n");
    return -1;     // Trả về lỗi cho hệ điều hành
  }

  /*
   * compile(argv[1]) thực hiện toàn bộ pipeline:
   *   - initReader()    : mở file
   *   - initScanner()   : khởi động bộ phân tích từ vựng
   *   - parseProgram()  : bắt đầu phân tích cú pháp
   *   - closeReader()   : đóng file
   *
   * Nếu compile() trả về IO_ERROR → file không tồn tại hoặc không đọc được.
   */
  if (compile(argv[1]) == IO_ERROR) {
    printf("Can't read input file!\n");
    return -1;
  }
    
  // Nếu không có lỗi → kết thúc bình thường
  return 0;
}