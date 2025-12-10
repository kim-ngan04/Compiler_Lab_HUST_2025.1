/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "error.h"

/*
 * Hàm error(...)
 * -------------------------------------------------------------------
 * Mục đích:
 *   - In ra thông báo lỗi phân tích từ vựng / cú pháp với vị trí lỗi.
 *   - Mỗi loại lỗi tương ứng với một ErrorCode (enum).
 *   - Dừng chương trình ngay khi phát hiện lỗi.
 *
 * Tham số:
 *   - err     : loại lỗi (ErrorCode)
 *   - lineNo  : số dòng xảy ra lỗi
 *   - colNo   : vị trí ký tự trong dòng
 *
 * Cách hoạt động:
 *   - Dùng switch-case để chọn chuỗi thông báo đúng (ERM_...)
 *   - In ra theo format: "line-col:message"
 *   - exit(0) để kết thúc chương trình.
 */
void error(ErrorCode err, int lineNo, int colNo) {
  switch (err) {

  // Lỗi: thiếu dấu "*)" kết thúc comment
  case ERR_ENDOFCOMMENT:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_ENDOFCOMMENT);
    break;

  // Lỗi: tên định danh quá dài
  case ERR_IDENTTOOLONG:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_IDENTTOOLONG);
    break;

  // Lỗi: hằng ký tự không hợp lệ (‘a’, ’1’,…)
  case ERR_INVALIDCHARCONSTANT:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDCHARCONSTANT);
    break;

  // Lỗi: gặp ký tự không thuộc bảng ký hiệu của ngôn ngữ
  case ERR_INVALIDSYMBOL:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDSYMBOL);
    break;

  // Lỗi: hằng số không hợp lệ (ví dụ: số quá dài, chứa ký tự sai)
  case ERR_INVALIDCONSTANT:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDCONSTANT);
    break;

  // Lỗi: sai kiểu dữ liệu
  case ERR_INVALIDTYPE:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDTYPE);
    break;

  // Lỗi: sai kiểu cơ bản (integer/char/boolean)
  case ERR_INVALIDBASICTYPE:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDBASICTYPE);
    break;

  // Lỗi: định nghĩa tham số không hợp lệ
  case ERR_INVALIDPARAM:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDPARAM);
    break;

  // Lỗi: câu lệnh không hợp lệ
  case ERR_INVALIDSTATEMENT:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDSTATEMENT);
    break;

  // Lỗi: truyền tham số sai khi gọi thủ tục/hàm
  case ERR_INVALIDARGUMENTS:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDARGUMENTS);
    break;

  // Lỗi: toán tử so sánh không hợp lệ
  case ERR_INVALIDCOMPARATOR:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDCOMPARATOR);
    break;

  // Lỗi: biểu thức sai cấu trúc
  case ERR_INVALIDEXPRESSION:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDEXPRESSION);
    break;

  // Lỗi: term (vế trong biểu thức) sai cấu trúc
  case ERR_INVALIDTERM:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDTERM);
    break;

  // Lỗi: factor (thành phần nhỏ nhất của biểu thức) sai
  case ERR_INVALIDFACTOR:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDFACTOR);
    break;

  // Lỗi: khai báo hằng sai cú pháp
  case ERR_INVALIDCONSTDECL:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDCONSTDECL);
    break;

  // Lỗi: khai báo kiểu sai cú pháp
  case ERR_INVALIDTYPEDECL:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDTYPEDECL);
    break;

  // Lỗi: khai báo biến sai cú pháp
  case ERR_INVALIDVARDECL:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDVARDECL);
    break;

  // Lỗi: khai báo chương trình con (procedure/function) sai cú pháp
  case ERR_INVALIDSUBDECL:
    printf("%d-%d:%s\n", lineNo, colNo, ERM_INVALIDSUBDECL);
    break;
  }

  // Dừng chương trình sau khi in lỗi
  exit(0);
}

/*
 * Hàm missingToken(...)
 * --------------------------------------------------------------------
 * Mục đích:
 *   - In ra lỗi khi parser mong đợi một token nào đó mà người lập trình quên.
 *   - Ví dụ parser mong đợi "THEN" nhưng gặp token khác → báo lỗi.
 *
 * Tham số:
 *   - tokenType : loại token bị thiếu
 *   - lineNo, colNo : vị trí lỗi
 *
 * Sử dụng tokenToString để in tên token bị thiếu.
 * Dừng chương trình bằng exit(0).
 */
void missingToken(TokenType tokenType, int lineNo, int colNo) {
  printf("%d-%d:Missing %s\n", lineNo, colNo, tokenToString(tokenType));
  exit(0);
}

/*
 * Hàm assert(...)
 * ---------------------------------------------------------------------
 * Mục đích:
 *   - In ra thông báo debug khi kiểm tra các điều kiện nội bộ.
 *   - Không dừng chương trình.
 *
 * Ghi chú:
 *   - Khác với assert() trong <assert.h>, hàm này chỉ in ra thông báo.
 */
void assert(char *msg) {
  printf("%s\n", msg);
}
