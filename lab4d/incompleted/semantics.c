/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * ============================================================================
 * MÔ TẢ FILE: semantics.c
 * ============================================================================
 * File semantics.c thực hiện phân tích ngữ nghĩa (semantic analysis) 
 * cho ngôn ngữ KPL. Đây là giai đoạn kiểm tra tính hợp lệ ngữ nghĩa 
 * của chương trình sau khi kiểm tra cú pháp.
 * 
 * CHỨC NĂNG CHÍNH:
 * - Tìm kiếm identifiers trong bảng ký hiệu (Symbol Table)
 * - Kiểm tra xem identifier có được khai báo không
 * - Kiểm tra xem identifier có bị trùng không
 * - Kiểm tra loại dữ liệu của các phần tử
 * - Kiểm tra tính nhất quán kiểu dữ liệu (type consistency)
 * - Giúp xây dựng cây cú pháp trừu tượng (AST)
 * 
 * KHÁI NIỆM QUAN TRỌNG:
 * 1. Symbol Table (Bảng ký hiệu): Cấu trúc dữ liệu lưu trữ thông tin 
 *    về tất cả các identifiers được khai báo
 * 
 * 2. Scope (Phạm vi): Mỗi block (chương trình, hàm, procedure) có một 
 *    scope riêng. Scope được tổ chức thành một danh sách liên kết.
 * 
 * 3. Object: Đại diện cho một identifier được khai báo (biến, hằng, 
 *    hàm, procedure, kiểu, tham số)
 * 
 * 4. Type: Đại diện cho kiểu dữ liệu (INTEGER, CHAR, ARRAY, ...)
 * 
 * 5. Scope Chain: Các scope được tổ chức thành một chuỗi, từ scope 
 *    hiện tại trở ra scope ngoài, để xử lý nested scopes.
 * 
 * QUYẾT TẮC TÌM KIẾM (Lookup):
 * - Tìm kiếm từ scope hiện tại (innermost)
 * - Nếu không tìm thấy, tìm trong scope ngoài
 * - Tiếp tục cho đến khi tìm thấy hoặc hết toàn bộ scopes
 * - Cuối cùng tìm trong global object list
 * 
 * BIẾN TOÀN CỤC:
 * - symtab: Con trỏ đến bảng ký hiệu toàn cục
 * - currentToken: Token hiện tại (để báo lỗi)
 */

#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "error.h"

extern SymTab* symtab;       // Bảng ký hiệu toàn cục
extern Token* currentToken;  // Token hiện tại (được sử dụng để báo lỗi)

/**
 * lookupObject(char *name)
 * -------------------------
 * Hàm tìm kiếm một object trong bảng ký hiệu dựa trên tên
 * 
 * Quy trình tìm kiếm (Scope Chain):
 * 1. Bắt đầu từ scope hiện tại (innermost scope)
 * 2. Tìm trong danh sách objects của scope hiện tại
 * 3. Nếu tìm thấy: trả về object
 * 4. Nếu không tìm thấy: chuyển đến scope ngoài (outer scope)
 * 5. Lặp lại bước 2-4 cho đến khi hết tất cả scopes
 * 6. Cuối cùng: tìm trong global object list
 * 
 * Điều này cho phép các identifiers trong scope ngoài có thể được truy cập
 * từ scope trong (nested scope), nhưng không ngược lại.
 * 
 * @param name: Tên của object cần tìm
 * @return: Con trỏ đến Object nếu tìm thấy, NULL nếu không
 */
Object* lookupObject(char *name) {
  Scope* scope = symtab->currentScope;  // Bắt đầu từ scope hiện tại
  Object* obj;

  // Tìm kiếm từ scope hiện tại cho đến scope global
  while (scope != NULL) {
    // Tìm object trong danh sách objects của scope
    obj = findObject(scope->objList, name);
    if (obj != NULL) return obj;  // Tìm thấy
    scope = scope->outer;  // Chuyển đến scope ngoài
  }
  
  // Cuối cùng tìm trong global object list
  obj = findObject(symtab->globalObjectList, name);
  if (obj != NULL) return obj;  // Tìm thấy
  
  return NULL;  // Không tìm thấy
}

/**
 * checkFreshIdent(char *name)
 * ----------------------------
 * Hàm kiểm tra xem một identifier có bị trùng trong scope hiện tại không
 * "Fresh" có nghĩa là "mới" - chưa được khai báo trước đó
 * 
 * Xử lý:
 * 1. Tìm kiếm name trong danh sách objects của scope hiện tại
 * 2. Nếu tìm thấy (không fresh): báo lỗi ERR_DUPLICATE_IDENT
 * 3. Nếu không tìm thấy (fresh): OK (không báo lỗi)
 * 
 * Lưu ý: Chỉ kiểm tra scope hiện tại, không kiểm tra scope ngoài
 * Điều này cho phép khai báo lại identifier trong scope con
 * 
 * @param name: Tên của identifier cần kiểm tra
 */
void checkFreshIdent(char *name) {
  // Kiểm tra trong scope hiện tại (chỉ scope này, không scope ngoài)
  if (findObject(symtab->currentScope->objList, name) != NULL)
    // Tìm thấy: báo lỗi trùng identifier
    error(ERR_DUPLICATE_IDENT, currentToken->lineNo, currentToken->colNo);
}

/**
 * checkDeclaredIdent(char* name)
 * -------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo không
 * 
 * Xử lý:
 * 1. Gọi lookupObject() để tìm identifier
 * 2. Nếu không tìm thấy: báo lỗi ERR_UNDECLARED_IDENT
 * 3. Nếu tìm thấy: trả về object
 * 
 * @param name: Tên của identifier
 * @return: Con trỏ đến Object nếu được khai báo
 */
Object* checkDeclaredIdent(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL) {
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_IDENT, currentToken->lineNo, currentToken->colNo);
  }
  return obj;
}

/**
 * checkDeclaredConstant(char* name)
 * ----------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo là CONSTANT không
 * 
 * Xử lý:
 * 1. Gọi lookupObject() để tìm identifier
 * 2. Nếu không tìm thấy: báo lỗi ERR_UNDECLARED_CONSTANT
 * 3. Nếu tìm thấy nhưng không phải CONSTANT: báo lỗi ERR_INVALID_CONSTANT
 * 4. Nếu tìm thấy và là CONSTANT: trả về object
 * 
 * @param name: Tên của hằng số
 * @return: Con trỏ đến Object constant
 */
Object* checkDeclaredConstant(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_CONSTANT, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_CONSTANT)
    // Tìm thấy nhưng không phải constant: báo lỗi
    error(ERR_INVALID_CONSTANT, currentToken->lineNo, currentToken->colNo);

  return obj;
}

/**
 * checkDeclaredType(char* name)
 * ------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo là TYPE không
 * 
 * Xử lý tương tự checkDeclaredConstant nhưng cho TYPE objects
 * 
 * @param name: Tên của kiểu dữ liệu
 * @return: Con trỏ đến Object type
 */
Object* checkDeclaredType(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_TYPE, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_TYPE)
    // Tìm thấy nhưng không phải type: báo lỗi
    error(ERR_INVALID_TYPE, currentToken->lineNo, currentToken->colNo);

  return obj;
}

/**
 * checkDeclaredVariable(char* name)
 * ----------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo là VARIABLE không
 * 
 * Xử lý:
 * 1. Gọi lookupObject() để tìm identifier
 * 2. Nếu không tìm thấy: báo lỗi ERR_UNDECLARED_VARIABLE
 * 3. Nếu tìm thấy nhưng không phải VARIABLE: báo lỗi ERR_INVALID_VARIABLE
 * 4. Nếu tìm thấy và là VARIABLE: trả về object
 * 
 * Được sử dụng khi: kiểm tra vế trái của câu lệnh gán, trong FOR statement
 * 
 * @param name: Tên của biến
 * @return: Con trỏ đến Object variable
 */
Object* checkDeclaredVariable(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_VARIABLE, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_VARIABLE)
    // Tìm thấy nhưng không phải variable: báo lỗi
    error(ERR_INVALID_VARIABLE, currentToken->lineNo, currentToken->colNo);

  return obj;
}

/**
 * checkDeclaredFunction(char* name)
 * ----------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo là FUNCTION không
 * 
 * Xử lý tương tự checkDeclaredVariable nhưng cho FUNCTION objects
 * 
 * @param name: Tên của hàm
 * @return: Con trỏ đến Object function
 */
Object* checkDeclaredFunction(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_FUNCTION, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_FUNCTION)
    // Tìm thấy nhưng không phải function: báo lỗi
    error(ERR_INVALID_FUNCTION, currentToken->lineNo, currentToken->colNo);

  return obj;
}

/**
 * checkDeclaredProcedure(char* name)
 * -----------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo là PROCEDURE không
 * 
 * Xử lý tương tự checkDeclaredFunction nhưng cho PROCEDURE objects
 * 
 * @param name: Tên của procedure
 * @return: Con trỏ đến Object procedure
 */
Object* checkDeclaredProcedure(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_PROCEDURE, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_PROCEDURE)
    // Tìm thấy nhưng không phải procedure: báo lỗi
    error(ERR_INVALID_PROCEDURE, currentToken->lineNo, currentToken->colNo);

  return obj;
}

/**
 * checkDeclaredLValueIdent(char* name)
 * ------------------------------------
 * Hàm kiểm tra xem một identifier có được khai báo là lvalue không
 * 
 * LValue (Left Value) là giá trị có thể đứng ở vế trái của câu lệnh gán
 * Có thể là:
 * - VARIABLE: biến
 * - PARAMETER: tham số (cả tham số giá trị và tham số tham chiếu)
 * - FUNCTION: tên hàm (chỉ nếu đang ở trong hàm đó để gán giá trị trả về)
 * 
 * Xử lý:
 * 1. Gọi lookupObject() để tìm identifier
 * 2. Nếu không tìm thấy: báo lỗi ERR_UNDECLARED_IDENT
 * 3. Kiểm tra loại object:
 *    - OBJ_VARIABLE: OK (là lvalue hợp lệ)
 *    - OBJ_PARAMETER: OK (là lvalue hợp lệ)
 *    - OBJ_FUNCTION: OK chỉ nếu là hàm hiện tại (để gán return value)
 *    - Các loại khác: báo lỗi ERR_INVALID_IDENT
 * 
 * @param name: Tên của identifier
 * @return: Con trỏ đến Object lvalue
 */
Object* checkDeclaredLValueIdent(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    // Không tìm thấy: báo lỗi
    error(ERR_UNDECLARED_IDENT, currentToken->lineNo, currentToken->colNo);

  switch (obj->kind) {
  case OBJ_VARIABLE:  // Biến: OK
  case OBJ_PARAMETER:  // Tham số: OK
    break;
  case OBJ_FUNCTION:  // Hàm: OK chỉ nếu là hàm hiện tại
    // Kiểm tra xem đây có phải là hàm hiện tại không
    // (để gán giá trị trả về)
    if (obj != symtab->currentScope->owner) 
      error(ERR_INVALID_IDENT, currentToken->lineNo, currentToken->colNo);
    break;
  default:  // Các loại khác (CONSTANT, TYPE, PROCEDURE): không OK
    error(ERR_INVALID_IDENT, currentToken->lineNo, currentToken->colNo);
  }

  return obj;
}

/**
 * checkIntType(Type* type)
 * ------------------------
 * Hàm kiểm tra xem một kiểu có phải là INTEGER không
 * 
 * Xử lý:
 * - Nếu type là INTEGER: OK (không báo lỗi)
 * - Nếu không: báo lỗi ERR_INVALID_BASICTYPE
 * 
 * Được sử dụng khi: kiểm tra vế phải của phép nhân/chia, 
 * index của mảng, biến trong FOR loop, ...
 * 
 * @param type: Kiểu cần kiểm tra
 */
void checkIntType(Type* type) {
  if (type->typeClass != TP_INT)
    // Không phải INTEGER: báo lỗi
    error(ERR_INVALID_BASICTYPE, currentToken->lineNo, currentToken->colNo);
}

/**
 * checkCharType(Type* type)
 * -------------------------
 * Hàm kiểm tra xem một kiểu có phải là CHAR không
 * 
 * Xử lý tương tự checkIntType nhưng cho kiểu CHAR
 * 
 * @param type: Kiểu cần kiểm tra
 */
void checkCharType(Type* type) {
  if (type->typeClass != TP_CHAR)
    // Không phải CHAR: báo lỗi
    error(ERR_INVALID_BASICTYPE, currentToken->lineNo, currentToken->colNo);
}

/**
 * checkBasicType(Type* type)
 * --------------------------
 * Hàm kiểm tra xem một kiểu có phải là kiểu cơ bản (INTEGER hoặc CHAR) không
 * 
 * Xử lý:
 * - Nếu type là INTEGER hoặc CHAR: OK
 * - Nếu không (ví dụ: ARRAY): báo lỗi ERR_INVALID_BASICTYPE
 * 
 * Được sử dụng khi: kiểm tra toán hạng của phép so sánh, điều kiện, ...
 * 
 * @param type: Kiểu cần kiểm tra
 */
void checkBasicType(Type* type) {
  if (type->typeClass != TP_INT && type->typeClass != TP_CHAR)
    // Không phải basic type: báo lỗi
    error(ERR_INVALID_BASICTYPE, currentToken->lineNo, currentToken->colNo);
}

/**
 * checkArrayType(Type* type)
 * ---------------------------
 * Hàm kiểm tra xem một kiểu có phải là ARRAY không
 * 
 * Xử lý:
 * - Nếu type là ARRAY: OK
 * - Nếu không: báo lỗi ERR_INVALID_TYPE
 * 
 * Được sử dụng khi: kiểm tra trước khi truy cập phần tử mảng
 * 
 * @param type: Kiểu cần kiểm tra
 */
void checkArrayType(Type* type) {
  if (type->typeClass != TP_ARRAY)
    // Không phải ARRAY: báo lỗi
    error(ERR_INVALID_TYPE, currentToken->lineNo, currentToken->colNo);
}

/**
 * checkTypeEquality(Type* type1, Type* type2)
 * -------------------------------------------
 * Hàm kiểm tra xem hai kiểu có bằng nhau không
 * 
 * Xử lý:
 * 1. Gọi compareType() để so sánh hai kiểu
 * 2. Nếu không bằng nhau: báo lỗi ERR_TYPE_INCONSISTENCY
 * 3. Nếu bằng nhau: OK (không báo lỗi)
 * 
 * Được sử dụng khi:
 * - Kiểm tra kiểu của assignment (lvalue = rvalue)
 * - Kiểm tra kiểu của arguments so với parameters
 * - Kiểm tra kiểu của toán hạng trong biểu thức
 * - Kiểm tra kiểu của các phần trong phép so sánh
 * 
 * @param type1: Kiểu thứ nhất
 * @param type2: Kiểu thứ hai
 */
void checkTypeEquality(Type* type1, Type* type2) {
  // So sánh hai kiểu
  if (!compareType(type1, type2))
    // Không bằng nhau: báo lỗi
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}
