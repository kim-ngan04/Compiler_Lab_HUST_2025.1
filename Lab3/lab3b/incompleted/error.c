/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * FILE: error.c
 * MỤC ĐÍCH:
 *   - Quản lý xử lý lỗi trong trình biên dịch
 *   - Ánh xạ mã lỗi đến thông báo lỗi
 *   - In thông báo lỗi với vị trí (dòng, cột)
 *   - Kết thúc chương trình khi phát hiện lỗi
 * 
 * DANH SÁCH LỖI:
 *   - Lỗi syntax: comment không đóng, identifier quá dài, v.v.
 *   - Lỗi semantic: biến không khai báo, type mismatch, v.v.
 */

#include <stdio.h>
#include <stdlib.h>
#include "error.h"

#define NUM_OF_ERRORS 29

/* Cấu trúc ErrorMessage: liên kết mã lỗi với thông báo */
struct ErrorMessage {
  ErrorCode errorCode;  /* Mã lỗi */
  char *message;        /* Thông báo lỗi */
};

/* Bảng ánh xạ tất cả các mã lỗi sang thông báo */
struct ErrorMessage errors[29] = {
  /* Lỗi quét (Scanner errors) */
  {ERR_END_OF_COMMENT, "End of comment expected."},
  {ERR_IDENT_TOO_LONG, "Identifier too long."},
  {ERR_INVALID_CONSTANT_CHAR, "Invalid char constant."},
  {ERR_INVALID_SYMBOL, "Invalid symbol."},
  
  /* Lỗi phân tích cú pháp (Parser errors) */
  {ERR_INVALID_IDENT, "An identifier expected."},
  {ERR_INVALID_CONSTANT, "A constant expected."},
  {ERR_INVALID_TYPE, "A type expected."},
  {ERR_INVALID_BASICTYPE, "A basic type expected."},
  {ERR_INVALID_VARIABLE, "A variable expected."},
  {ERR_INVALID_FUNCTION, "A function identifier expected."},
  {ERR_INVALID_PROCEDURE, "A procedure identifier expected."},
  {ERR_INVALID_PARAMETER, "A parameter expected."},
  {ERR_INVALID_STATEMENT, "Invalid statement."},
  {ERR_INVALID_COMPARATOR, "A comparator expected."},
  {ERR_INVALID_EXPRESSION, "Invalid expression."},
  {ERR_INVALID_TERM, "Invalid term."},
  {ERR_INVALID_FACTOR, "Invalid factor."},
  {ERR_INVALID_LVALUE, "Invalid lvalue in assignment."},
  {ERR_INVALID_ARGUMENTS, "Wrong arguments."},
  
  /* Lỗi kiểm tra ngữ nghĩa (Semantic errors) */
  {ERR_UNDECLARED_IDENT, "Undeclared identifier."},
  {ERR_UNDECLARED_CONSTANT, "Undeclared constant."},
  {ERR_UNDECLARED_INT_CONSTANT, "Undeclared integer constant."},
  {ERR_UNDECLARED_TYPE, "Undeclared type."},
  {ERR_UNDECLARED_VARIABLE, "Undeclared variable."},
  {ERR_UNDECLARED_FUNCTION, "Undeclared function."},
  {ERR_UNDECLARED_PROCEDURE, "Undeclared procedure."},
  {ERR_DUPLICATE_IDENT, "Duplicate identifier."},
  {ERR_TYPE_INCONSISTENCY, "Type inconsistency"},
  {ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, "The number of arguments and the number of parameters are inconsistent."}
};

/* 
 * HÀM error: In thông báo lỗi và kết thúc chương trình
 * 
 * THAM SỐ:
 *   - err: mã lỗi (ErrorCode)
 *   - lineNo: số dòng nơi lỗi xảy ra
 *   - colNo: số cột nơi lỗi xảy ra
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Duyệt mảng errors[] để tìm mã lỗi err
 *   2. Khi tìm thấy:
 *      - In "lineNo-colNo:thông báo lỗi"
 *      - Gọi exit(0) để kết thúc chương trình
 * 
 * VÍ DỤ:
 *   error(ERR_IDENT_TOO_LONG, 5, 10);
 *   // In: "5-10:Identifier too long."
 *   // Rồi thoát chương trình
 */
void error(ErrorCode err, int lineNo, int colNo) {
  int i;
  // Tìm thông báo lỗi tương ứng
  for (i = 0 ; i < NUM_OF_ERRORS; i ++) 
    if (errors[i].errorCode == err) {
      // In thông báo lỗi với vị trí
      printf("%d-%d:%s\n", lineNo, colNo, errors[i].message);
      exit(0);  // Kết thúc chương trình
    }
}

/* 
 * HÀM missingToken: In lỗi token bị thiếu
 * 
 * THAM SỐ:
 *   - tokenType: loại token được mong đợi
 *   - lineNo: số dòng
 *   - colNo: số cột
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Gọi tokenToString(tokenType) để lấy tên token
 *   2. In thông báo "Missing [tên token]"
 *   3. Kết thúc chương trình
 * 
 * VÍ DỤ:
 *   missingToken(SB_SEMICOLON, 10, 5);
 *   // In: "10-5:Missing SB_SEMICOLON"
 */
void missingToken(TokenType tokenType, int lineNo, int colNo) {
  printf("%d-%d:Missing %s\n", lineNo, colNo, tokenToString(tokenType));
  exit(0);  // Kết thúc chương trình
}

/* 
 * HÀM assert: In thông báo debug
 * 
 * THAM SỐ:
 *   - msg: thông báo cần in
 * 
 * CÁCH HOẠT ĐỘNG:
 *   - In thông báo msg ra màn hình
 *   - Dùng để debug trong quá trình phát triển
 * 
 * VÍ DỤ:
 *   assert("Entering compileExpression");
 *   // In: "Entering compileExpression"
 */
void assert(char *msg) {
  printf("%s\n", msg);
}
