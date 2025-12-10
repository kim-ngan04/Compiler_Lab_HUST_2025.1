/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

/* 
 * File debug: in ra cấu trúc Symbol Table
 * (Type, Object, Scope...)
 */

#include <stdio.h>
#include "debug.h"

/* ---------------------------------------------------
 * Hàm pad: In khoảng trắng để căn lề
 * ---------------------------------------------------
 * Mục đích:
 *   - In ra n dấu cách liên tiếp
 *   - Dùng để thụt lề (indentation) khi in các scope lồng nhau
 *   - Giúp dễ nhìn cấu trúc phân cấp của bảng ký hiệu
 *
 * Tham số:
 *   - n: số dấu cách cần in ra
 *
 * Ví dụ: pad(4) sẽ in ra 4 dấu cách
 */
void pad(int n) {
  int i;
  // Vòng lặp in ra n dấu cách
  for (i = 0; i < n; i++) {
    printf(" ");
  }
}

/* ---------------------------------------------------
 * Hàm printType: In kiểu dữ liệu
 * ---------------------------------------------------
 * Mục đích:
 *   - In ra thông tin kiểu dữ liệu dưới dạng dễ đọc
 *   - Hỗ trợ 3 loại kiểu: Int, Char, Array
 *
 * Tham số:
 *   - type: con trỏ đến cấu trúc Type
 *
 * Ví dụ:
 *   - Int      → in ra "Int"
 *   - Char     → in ra "Char"
 *   - Array    → in ra "Arr(10, Int)" (mảng 10 phần tử kiểu Int)
 */
void printType(Type* type) {
  switch (type->typeClass) {

  case TP_INT:
    // Kiểu nguyên
    printf("Int");
    break;

  case TP_CHAR:
    // Kiểu ký tự
    printf("Char");
    break;

  case TP_ARRAY:
    // Kiểu mảng: in ra kích thước và kiểu phần tử (đệ quy)
    // Ví dụ: Arr(10, Int) hoặc Arr(10, Arr(5, Int)) cho mảng 2 chiều
    printf("Arr(%d,", type->arraySize);
    printType(type->elementType);  // Gọi đệ quy để xử lý kiểu phần tử
    printf(")");
    break;
  }
}

/* ---------------------------------------------------
 * Hàm printConstantValue: In giá trị hằng số
 * ---------------------------------------------------
 * Mục đích:
 *   - In ra giá trị cụ thể của hằng số (Int hoặc Char)
 *
 * Tham số:
 *   - value: con trỏ đến cấu trúc ConstantValue
 *
 * Ví dụ:
 *   - Hằng số Int: in ra "10"
 *   - Hằng số Char: in ra "'a'"
 */
void printConstantValue(ConstantValue* value) {
  switch (value->type) {

  case TP_INT:
    // In giá trị nguyên
    printf("%d", value->intValue);
    break;

  case TP_CHAR:
    // In giá trị ký tự với dấu ngoặc đơn
    printf("\'%c\'", value->charValue);
    break;

  default:
    break;
  }
}

/* ---------------------------------------------------
 * Hàm printObject: In thông tin của một Object
 * ---------------------------------------------------
 * Mục đích:
 *   - In ra thông tin chi tiết của một đối tượng trong bảng ký hiệu
 *   - Hỗ trợ 7 loại object khác nhau
 *   - Căn lề theo mức độ sâu của scope (indent)
 *
 * Tham số:
 *   - obj: con trỏ đến đối tượng cần in
 *   - indent: mức độ thụt lề (số dấu cách)
 *
 * Ví dụ:
 *   - Hằng số: "Const c1 = 10"
 *   - Biến: "Var v1 : Int"
 *   - Tham số: "Param p1 : Int" hoặc "Param VAR p2 : Char"
 *   - Hàm: "Function f : Int" + in nội dung scope
 */
void printObject(Object* obj, int indent) {
  switch (obj->kind) {

  /* ===== CONSTANT ===== */
  case OBJ_CONSTANT:
    // In tên và giá trị hằng số
    pad(indent);
    printf("Const %s = ", obj->name);
    printConstantValue(obj->constAttrs->value);
    break;

  /* ===== TYPE ===== */
  case OBJ_TYPE:
    // In tên kiểu và định nghĩa kiểu
    pad(indent);
    printf("Type %s = ", obj->name);
    printType(obj->typeAttrs->actualType);
    break;

  /* ===== VARIABLE ===== */
  case OBJ_VARIABLE:
    // In tên biến và kiểu của nó
    pad(indent);
    printf("Var %s : ", obj->name);
    printType(obj->varAttrs->type);
    break;

  /* ===== PARAMETER ===== */
  case OBJ_PARAMETER:
    // In tên tham số, kiểu truyền, và kiểu dữ liệu
    pad(indent);

    // Kiểm tra kiểu truyền tham số: truyền tham trị hay truyền tham biến
    if (obj->paramAttrs->kind == PARAM_VALUE)
      printf("Param %s : ", obj->name);      // Truyền tham trị (pass by value)
    else
      printf("Param VAR %s : ", obj->name);  // Truyền tham biến (pass by reference)

    printType(obj->paramAttrs->type);
    break;

  /* ===== FUNCTION ===== */
  case OBJ_FUNCTION:
    // In tên hàm và kiểu trả về, sau đó in scope bên trong hàm
    pad(indent);
    printf("Function %s : ", obj->name);
    printType(obj->funcAttrs->returnType);
    printf("\n");

    // In ra danh sách các tham số và biến cục bộ của hàm
    // Thụt lề thêm 4 dấu cách cho các phần tử bên trong scope
    printScope(obj->funcAttrs->scope, indent + 4);
    break;

  /* ===== PROCEDURE ===== */
  case OBJ_PROCEDURE:
    // In tên procedure, sau đó in scope bên trong procedure
    pad(indent);
    printf("Procedure %s\n", obj->name);

    // In ra danh sách các tham số và biến cục bộ của procedure
    // Thụt lề thêm 4 dấu cách cho các phần tử bên trong scope
    printScope(obj->procAttrs->scope, indent + 4);
    break;

  /* ===== PROGRAM ===== */
  case OBJ_PROGRAM:
    // In tên chương trình, sau đó in scope toàn chương trình
    pad(indent);
    printf("Program %s\n", obj->name);

    // In ra danh sách tất cả các khai báo toàn cục của chương trình
    // Thụt lề thêm 4 dấu cách cho các phần tử bên trong scope
    printScope(obj->progAttrs->scope, indent + 4);
    break;
  }
}

/* ---------------------------------------------------
 * Hàm printObjectList: In danh sách các Object
 * ---------------------------------------------------
 * Mục đích:
 *   - Duyệt qua danh sách liên kết các object và in từng object
 *   - Được gọi bởi printScope để in tất cả object trong một scope
 *
 * Tham số:
 *   - objList: con trỏ đến node đầu tiên của danh sách liên kết
 *   - indent: mức độ thụt lề
 *
 * Cách hoạt động:
 *   - Bắt đầu từ node đầu (objList)
 *   - In từng object bằng gọi printObject
 *   - Di chuyển đến node tiếp theo (node->next)
 *   - Dừng khi gặp NULL (hết danh sách)
 */
void printObjectList(ObjectNode* objList, int indent) {
  ObjectNode* node = objList;

  // Duyệt danh sách liên kết cho đến khi gặp NULL
  while (node != NULL) {
    printObject(node->object, indent);
    printf("\n");
    node = node->next;  // Di chuyển đến node tiếp theo
  }
}

/* ---------------------------------------------------
 * Hàm printScope: In tất cả object trong một scope
 * ---------------------------------------------------
 * Mục đích:
 *   - Là một hàm wrapper để in danh sách object của một scope
 *   - Gọi printObjectList với danh sách object của scope
 *
 * Tham số:
 *   - scope: con trỏ đến Scope cần in
 *   - indent: mức độ thụt lề
 *
 * Cách hoạt động:
 *   - Lấy danh sách object từ scope->objList
 *   - Gọi printObjectList để in từng object
 */
void printScope(Scope* scope, int indent) {
  // In danh sách object trong scope này
  printObjectList(scope->objList, indent);
}