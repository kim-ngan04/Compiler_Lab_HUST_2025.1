/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "symtab.h"
#include "debug.h"

extern SymTab* symtab;
/******************************************************************/

int main() {
  Object* obj;

  // === KHỞI TẠO BẢNG KÝ HIỆU ===
  // Khởi tạo bảng ký hiệu (symbol table) trống
  initSymTab();

  // === KHỐI CHƯƠNG TRÌNH (PROGRAM BLOCK) ===
  // Tạo đối tượng chương trình với tên "PRG"
  obj = createProgramObject("PRG");
  // Vào khối chương trình: đẩy phạm vi của chương trình lên stack
  // Từ đây trở đi, tất cả khai báo sẽ thuộc phạm vi của chương trình
  enterBlock(obj->progAttrs->scope);

  // --- Khai báo hằng số c1 = 10 ---
  obj = createConstantObject("c1");
  obj->constAttrs->value = makeIntConstant(10);  // Gán giá trị hằng số
  declareObject(obj);  // Đăng ký hằng số vào bảng ký hiệu

  // --- Khai báo hằng số c2 = 'a' ---
  obj = createConstantObject("c2");
  obj->constAttrs->value = makeCharConstant('a');
  declareObject(obj);

  // --- Khai báo kiểu t1: mảng 10 phần tử kiểu integer ---
  obj = createTypeObject("t1");
  obj->typeAttrs->actualType = makeArrayType(10, makeIntType());
  declareObject(obj);

  // --- Khai báo biến v1: kiểu integer ---
  obj = createVariableObject("v1");
  obj->varAttrs->type = makeIntType();
  declareObject(obj);

  // --- Khai báo biến v2: mảng 2 chiều 10x10 kiểu integer ---
  obj = createVariableObject("v2");
  obj->varAttrs->type = makeArrayType(10, makeArrayType(10, makeIntType()));
  declareObject(obj);

  // === KHỐI HÀM (FUNCTION BLOCK) ===
  // --- Khai báo hàm f với kiểu trả về là integer ---
  obj = createFunctionObject("f");
  obj->funcAttrs->returnType = makeIntType();
  declareObject(obj);
  
  // Vào khối hàm: tạo phạm vi cục bộ cho hàm f
  enterBlock(obj->funcAttrs->scope);
 
  // --- Khai báo tham số p1: truyền giá trị (pass by value), kiểu integer ---
  obj = createParameterObject("p1", PARAM_VALUE, symtab->currentScope->owner);
  obj->paramAttrs->type = makeIntType();
  declareObject(obj);

  // --- Khai báo tham số p2: truyền tham chiếu (pass by reference), kiểu char ---
  obj = createParameterObject("p2", PARAM_REFERENCE, symtab->currentScope->owner);
  obj->paramAttrs->type = makeCharType();
  declareObject(obj);

  // Thoát khối hàm: đưa phạm vi quay lại chương trình chính
  exitBlock();

  // === KHỐI THỦ TỤC (PROCEDURE BLOCK) ===
  // --- Khai báo thủ tục p ---
  obj = createProcedureObject("p");
  declareObject(obj);
  
  // Vào khối thủ tục: tạo phạm vi cục bộ cho thủ tục p
  enterBlock(obj->procAttrs->scope);
 
  // --- Khai báo tham số v1: truyền giá trị, kiểu integer ---
  obj = createParameterObject("v1", PARAM_VALUE, symtab->currentScope->owner);
  obj->paramAttrs->type = makeIntType();
  declareObject(obj);

  // --- Khai báo hằng số c1 = 'a' trong phạm vi thủ tục ---
  // Lưu ý: c1 này khác với c1 ở chương trình chính (shadowing)
  obj = createConstantObject("c1");
  obj->constAttrs->value = makeCharConstant('a');
  declareObject(obj);
    
  // --- Khai báo hằng số c3 = 10 trong phạm vi thủ tục ---
  obj = createConstantObject("c3");
  obj->constAttrs->value = makeIntConstant(10);
  declareObject(obj);

  // --- Khai báo kiểu t1 = integer trong phạm vi thủ tục ---
  obj = createTypeObject("t1");
  obj->typeAttrs->actualType = makeIntType();
  declareObject(obj);

  // --- Khai báo kiểu t2: mảng 10 phần tử kiểu integer trong phạm vi thủ tục ---
  obj = createTypeObject("t2");
  obj->typeAttrs->actualType = makeArrayType(10, makeIntType());
  declareObject(obj);

  // --- Khai báo biến v2: mảng 10 phần tử kiểu integer trong phạm vi thủ tục ---
  obj = createVariableObject("v2");
  obj->varAttrs->type = makeArrayType(10, makeIntType());
  declareObject(obj);

  // --- Khai báo biến v3: kiểu char trong phạm vi thủ tục ---
  obj = createVariableObject("v3");
  obj->varAttrs->type = makeCharType();
  declareObject(obj);

  // Thoát khối thủ tục: đưa phạm vi quay lại chương trình chính
  exitBlock();

  // === HOÀN TẤT ===
  // Thoát khối chương trình: quay lại phạm vi global
  exitBlock();
  
  // In ra cấu trúc bảng ký hiệu để kiểm tra kết quả
  // Tham số thứ 2 (0) là mức độ thụt lề ban đầu
  printObject(symtab->program, 0);
  
  // Giải phóng bộ nhớ của bảng ký hiệu
  cleanSymTab();
    
  return 0;
}
