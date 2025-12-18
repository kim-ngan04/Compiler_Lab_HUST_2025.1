/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * FILE: symtab.c
 * MỤC ĐÍCH:
 *   - Quản lý bảng ký hiệu (symbol table)
 *   - Tạo và quản lý các object (biến, hằng, kiểu, hàm, thủ tục)
 *   - Quản lý scope (phạm vi) của các khai báo
 *   - Kiểm tra định nghĩa (semantic checking)
 * 
 * CẤU TRÚC CHÍNH:
 *   - Type: biểu diễn kiểu dữ liệu (Int, Char, Array)
 *   - Object: biểu diễn các khai báo (hằng, biến, kiểu, hàm, thủ tục)
 *   - Scope: phạm vi khai báo (toàn cục, cục bộ hàm, etc.)
 *   - SymTab: bảng ký hiệu chính
 * 
 * CÁC HÀM CHÍNH:
 *   - Hàm tạo Type: makeIntType(), makeCharType(), makeArrayType()
 *   - Hàm tạo Object: createConstantObject(), createVariableObject(), etc.
 *   - Hàm kiểm tra: lookupObject(), checkFreshIdent(), checkDeclaredConstant()
 *   - Hàm quản lý scope: enterBlock(), exitBlock(), declareObject()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "error.h"

/* Khai báo trước các hàm để quản lý bộ nhớ */
void freeObject(Object* obj);
void freeScope(Scope* scope);
void freeObjectList(ObjectNode *objList);
void freeReferenceList(ObjectNode *objList);

/* Biến global: bảng ký hiệu chính */
SymTab* symtab;
/* Các kiểu cơ bản được chia sẻ */
Type* intType;
Type* charType;
/* Con trỏ đến token hiện tại (từ parser.c) */
extern Token *currentToken;

/******* CÁC HÀM TIỆN ÍCH VỀ KIỂU DỮ LIỆU *******/

/* 
 * HÀM makeIntType: Tạo kiểu Int
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ cho Type
 *   2. Đặt typeClass = TP_INT
 *   3. Trả về con trỏ
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Type mới
 */
Type* makeIntType(void) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_INT;
  return type;
}

/* 
 * HÀM makeCharType: Tạo kiểu Char
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ cho Type
 *   2. Đặt typeClass = TP_CHAR
 *   3. Trả về con trỏ
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Type mới
 */
Type* makeCharType(void) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_CHAR;
  return type;
}

/* 
 * HÀM makeArrayType: Tạo kiểu mảng
 * 
 * THAM SỐ:
 *   - arraySize: kích thước mảng
 *   - elementType: con trỏ đến kiểu phần tử
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ cho Type
 *   2. Đặt typeClass = TP_ARRAY
 *   3. Lưu arraySize và elementType
 *   4. Trả về con trỏ
 * 
 * MỤC ĐÍCH:
 *   - Tạo kiểu mảng lồng nhau (hỗ trợ mảng 2D, 3D, ...)
 * 
 * VÍ DỤ:
 *   Type* arr1d = makeArrayType(10, makeIntType());
 *   Type* arr2d = makeArrayType(10, makeArrayType(5, makeIntType()));
 */
Type* makeArrayType(int arraySize, Type* elementType) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_ARRAY;
  type->arraySize = arraySize;
  type->elementType = elementType;
  return type;
}

/* 
 * HÀM duplicateType: Sao chép kiểu dữ liệu
 * 
 * THAM SỐ:
 *   - type: con trỏ đến Type cần sao chép
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ mới cho Type
 *   2. Copy typeClass
 *   3. Nếu là mảng → sao chép kích thước và elementType (đệ quy)
 *   4. Trả về bản sao
 * 
 * MỤC ĐÍCH:
 *   - Tạo bản sao độc lập (deep copy) để tránh chia sẻ con trỏ
 *   - Xử lý đệ quy cho mảng lồng nhau
 * 
 * VÍ DỤ:
 *   Type* original = makeArrayType(10, makeIntType());
 *   Type* copy = duplicateType(original);
 *   // copy là bản sao độc lập
 */
Type* duplicateType(Type* type) {
  Type* resultType = (Type*) malloc(sizeof(Type));
  resultType->typeClass = type->typeClass;
  if (type->typeClass == TP_ARRAY) {
    resultType->arraySize = type->arraySize;
    /* Gọi đệ quy để sao chép elementType */
    resultType->elementType = duplicateType(type->elementType);
  }
  return resultType;
}

/* 
 * HÀM compareType: So sánh hai kiểu dữ liệu
 * 
 * THAM SỐ:
 *   - type1, type2: hai con trỏ đến Type cần so sánh
 * 
 * TRẢ VỀ:
 *   - 1: nếu hai type giống nhau
 *   - 0: nếu khác nhau
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. So sánh typeClass
 *   2. Nếu khác → return 0 (khác)
 *   3. Nếu giống:
 *      - Int/Char → giống nhau, return 1
 *      - Array → so sánh arraySize, rồi elementType (đệ quy)
 * 
 * MỤC ĐÍCH:
 *   - Kiểm tra kiểu chặt chẽ (type checking)
 *   - Đảm bảo tính nhất quán kiểu trong gán, tham số, etc.
 * 
 * VÍ DỤ:
 *   int a = compareType(makeIntType(), makeIntType());  // a = 1
 *   int b = compareType(makeIntType(), makeCharType()); // b = 0
 */
int compareType(Type* type1, Type* type2) {
  if (type1->typeClass == type2->typeClass) {
    if (type1->typeClass == TP_ARRAY) {
      /* Array: kiểm tra kích thước và elementType */
      if (type1->arraySize == type2->arraySize)
        /* Gọi đệ quy để so sánh elementType */
	return compareType(type1->elementType, type2->elementType);
      else return 0;  /* Kích thước khác */
    } else 
      return 1;  /* Int hoặc Char giống nhau */
  } else 
    return 0;  /* TypeClass khác */
}

/* 
 * HÀM freeType: Giải phóng bộ nhớ của Type
 * 
 * THAM SỐ:
 *   - type: con trỏ đến Type cần giải phóng
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Nếu TP_INT hoặc TP_CHAR → free type
 *   2. Nếu TP_ARRAY:
 *      - Gọi đệ quy freeType(elementType) trước
 *      - Rồi free type
 * 
 * MỤC ĐÍCH:
 *   - Tránh rò rỉ bộ nhớ (memory leak)
 *   - Xử lý đệ quy cho mảng lồng nhau
 */
void freeType(Type* type) {
  switch (type->typeClass) {
  case TP_INT:
  case TP_CHAR:
    free(type);
    break;
  case TP_ARRAY:
    /* Free elementType trước (bottom-up) */
    freeType(type->elementType);
    free(type);
    break;
  }
}

/******* CÁC HÀM TIỆN ÍCH VỀ GIÁ TRỊ HẰNG SỐ *******/

/* 
 * HÀM makeIntConstant: Tạo hằng số nguyên
 * 
 * THAM SỐ:
 *   - i: giá trị số nguyên
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến ConstantValue mới
 * 
 * VÍ DỤ:
 *   ConstantValue* val = makeIntConstant(10);
 *   // val->type = TP_INT, val->intValue = 10
 */
ConstantValue* makeIntConstant(int i) {
  ConstantValue* value = (ConstantValue*) malloc(sizeof(ConstantValue));
  value->type = TP_INT;
  value->intValue = i;
  return value;
}

/* 
 * HÀM makeCharConstant: Tạo hằng số ký tự
 * 
 * THAM SỐ:
 *   - ch: giá trị ký tự
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến ConstantValue mới
 * 
 * VÍ DỤ:
 *   ConstantValue* val = makeCharConstant('a');
 *   // val->type = TP_CHAR, val->charValue = 'a'
 */
ConstantValue* makeCharConstant(char ch) {
  ConstantValue* value = (ConstantValue*) malloc(sizeof(ConstantValue));
  value->type = TP_CHAR;
  value->charValue = ch;
  return value;
}

/* 
 * HÀM duplicateConstantValue: Sao chép giá trị hằng số
 * 
 * THAM SỐ:
 *   - v: con trỏ đến ConstantValue cần sao chép
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến ConstantValue sao chép (bản sao độc lập)
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ mới
 *   2. Copy type
 *   3. Copy giá trị (intValue hoặc charValue)
 *   4. Trả về bản sao
 */
ConstantValue* duplicateConstantValue(ConstantValue* v) {
  ConstantValue* value = (ConstantValue*) malloc(sizeof(ConstantValue));
  value->type = v->type;
  if (v->type == TP_INT) 
    value->intValue = v->intValue;
  else
    value->charValue = v->charValue;
  return value;
}

/******* CÁC HÀM VỀ PHẠM VI VÀ ĐỐI TƯỢNG *******/

/* 
 * HÀM createScope: Tạo phạm vi (scope) mới
 * 
 * THAM SỐ:
 *   - owner: con trỏ đến object sở hữu scope (hàm/thủ tục/chương trình)
 *   - outer: con trỏ đến scope bên ngoài (parent scope), NULL nếu scope toàn cục
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Scope mới
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ cho Scope
 *   2. Khởi tạo objList = NULL (danh sách object ban đầu trống)
 *   3. Lưu owner và outer
 *   4. Trả về con trỏ
 * 
 * MỤC ĐÍCH:
 *   - Tạo mối quan hệ phạm vi lồng nhau (nested scope)
 *   - Cho phép tìm kiếm biến theo chiều sâu từ innermost đến outermost
 * 
 * VÍ DỤ:
 *   // Scope toàn cục
 *   Scope* globalScope = createScope(program, NULL);
 *   
 *   // Scope cục bộ hàm (outer = globalScope)
 *   Scope* funcScope = createScope(function, globalScope);
 */
Scope* createScope(Object* owner, Scope* outer) {
  Scope* scope = (Scope*) malloc(sizeof(Scope));
  scope->objList = NULL;  /* Danh sách object ban đầu trống */
  scope->owner = owner;   /* Chủ sở hữu scope */
  scope->outer = outer;   /* Scope bên ngoài (để tìm kiếm) */
  return scope;
}

/******* CÁC HÀM TẠO OBJECT *******/

/* 
 * HÀM createProgramObject: Tạo object chương trình
 * 
 * THAM SỐ:
 *   - programName: tên chương trình
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object chương trình
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ Object
 *   2. Lưu tên chương trình
 *   3. Đặt kind = OBJ_PROGRAM
 *   4. Cấp phát progAttrs
 *   5. Tạo scope toàn cục (outer = NULL)
 *   6. Lưu vào symtab->program
 *   7. Trả về object
 * 
 * MỤC ĐÍCH:
 *   - Tạo root object cho toàn bộ bảng ký hiệu
 *   - Khởi tạo scope toàn cục
 */
Object* createProgramObject(char *programName) {
  Object* program = (Object*) malloc(sizeof(Object));
  strcpy(program->name, programName);
  program->kind = OBJ_PROGRAM;
  program->progAttrs = (ProgramAttributes*) malloc(sizeof(ProgramAttributes));
  program->progAttrs->scope = createScope(program, NULL);  /* Scope toàn cục */
  symtab->program = program;  /* Ghi vào bảng ký hiệu */

  return program;
}

/* 
 * HÀM createConstantObject: Tạo object hằng số
 * 
 * THAM SỐ:
 *   - name: tên hằng số
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object hằng số
 * 
 * GHI CHÚ:
 *   - Sau tạo, cần gán giá trị: obj->constAttrs->value = makeIntConstant(10);
 */
Object* createConstantObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_CONSTANT;
  obj->constAttrs = (ConstantAttributes*) malloc(sizeof(ConstantAttributes));
  return obj;
}

/* 
 * HÀM createTypeObject: Tạo object kiểu dữ liệu
 * 
 * THAM SỐ:
 *   - name: tên kiểu
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object kiểu
 * 
 * GHI CHÚ:
 *   - Sau tạo, cần gán kiểu thực tế: obj->typeAttrs->actualType = makeArrayType(...);
 */
Object* createTypeObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_TYPE;
  obj->typeAttrs = (TypeAttributes*) malloc(sizeof(TypeAttributes));
  return obj;
}

/* 
 * HÀM createVariableObject: Tạo object biến
 * 
 * THAM SỐ:
 *   - name: tên biến
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object biến
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ Object
 *   2. Lưu tên biến
 *   3. Đặt kind = OBJ_VARIABLE
 *   4. Cấp phát varAttrs
 *   5. Lưu scope hiện tại (nơi biến được khai báo)
 * 
 * GHI CHÚ:
 *   - Sau tạo, cần gán kiểu: obj->varAttrs->type = makeIntType();
 */
Object* createVariableObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_VARIABLE;
  obj->varAttrs = (VariableAttributes*) malloc(sizeof(VariableAttributes));
  obj->varAttrs->scope = symtab->currentScope;  /* Lưu scope hiện tại */
  return obj;
}

/* 
 * HÀM createFunctionObject: Tạo object hàm
 * 
 * THAM SỐ:
 *   - name: tên hàm
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object hàm
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ Object
 *   2. Lưu tên hàm
 *   3. Đặt kind = OBJ_FUNCTION
 *   4. Cấp phát funcAttrs
 *   5. Khởi tạo paramList = NULL
 *   6. Tạo scope cục bộ (outer = currentScope)
 * 
 * GHI CHÚ:
 *   - Scope của hàm là con của currentScope (quản lý biến cục bộ)
 *   - Sau tạo, cần gán kiểu trả về: obj->funcAttrs->returnType = makeIntType();
 */
Object* createFunctionObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_FUNCTION;
  obj->funcAttrs = (FunctionAttributes*) malloc(sizeof(FunctionAttributes));
  obj->funcAttrs->paramList = NULL;  /* Danh sách tham số ban đầu trống */
  obj->funcAttrs->scope = createScope(obj, symtab->currentScope);  /* Scope lồng */
  return obj;
}

/* 
 * HÀM createProcedureObject: Tạo object thủ tục
 * 
 * THAM SỐ:
 *   - name: tên thủ tục
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object thủ tục
 * 
 * GHI CHÚ:
 *   - Thủ tục giống hàm nhưng không có kiểu trả về
 *   - Scope của thủ tục là con của currentScope
 */
Object* createProcedureObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_PROCEDURE;
  obj->procAttrs = (ProcedureAttributes*) malloc(sizeof(ProcedureAttributes));
  obj->procAttrs->paramList = NULL;  /* Danh sách tham số ban đầu trống */
  obj->procAttrs->scope = createScope(obj, symtab->currentScope);  /* Scope lồng */
  return obj;
}

/* 
 * HÀM createParameterObject: Tạo object tham số
 * 
 * THAM SỐ:
 *   - name: tên tham số
 *   - kind: loại tham số (PARAM_VALUE hoặc PARAM_REFERENCE)
 *   - owner: con trỏ đến hàm/thủ tục chứa tham số
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến Object tham số
 * 
 * GHI CHÚ:
 *   - Sau tạo, cần gán kiểu: obj->paramAttrs->type = makeIntType();
 */
Object* createParameterObject(char *name, enum ParamKind kind, Object* owner) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_PARAMETER;
  obj->paramAttrs = (ParameterAttributes*) malloc(sizeof(ParameterAttributes));
  obj->paramAttrs->kind = kind;  /* Loại tham số */
  obj->paramAttrs->function = owner;  /* Hàm/thủ tục chứa tham số */
  return obj;
}

void freeObject(Object* obj) {
  /* 
   * HÀM freeObject: Giải phóng bộ nhớ của object
   * 
   * THAM SỐ:
   *   - obj: con trỏ đến object cần giải phóng
   * 
   * CÁCH HOẠT ĐỘNG:
   *   1. Kiểm tra loại object
   *   2. Tùy theo loại, giải phóng các thuộc tính riêng:
   *      - OBJ_CONSTANT: giải phóng giá trị hằng
   *      - OBJ_TYPE: giải phóng actualType (kiểu thực tế)
   *      - OBJ_VARIABLE: giải phóng kiểu biến
   *      - OBJ_FUNCTION: giải phóng danh sách tham số, kiểu trả về, scope
   *      - OBJ_PROCEDURE: giải phóng danh sách tham số và scope
   *      - OBJ_PROGRAM: giải phóng scope toàn cục
   *      - OBJ_PARAMETER: giải phóng kiểu tham số
   *   3. Giải phóng bộ nhớ object chính
   * 
   * MỤC ĐÍCH:
   *   - Tránh rò rỉ bộ nhớ (memory leak)
   *   - Dọn dẹp khi kết thúc scope hoặc chương trình
   */
  switch (obj->kind) {
  case OBJ_CONSTANT:
    free(obj->constAttrs->value);
    free(obj->constAttrs);
    break;
  case OBJ_TYPE:
    free(obj->typeAttrs->actualType);
    free(obj->typeAttrs);
    break;
  case OBJ_VARIABLE:
    free(obj->varAttrs->type);
    free(obj->varAttrs);
    break;
  case OBJ_FUNCTION:
    freeReferenceList(obj->funcAttrs->paramList);
    freeType(obj->funcAttrs->returnType);
    freeScope(obj->funcAttrs->scope);
    free(obj->funcAttrs);
    break;
  case OBJ_PROCEDURE:
    freeReferenceList(obj->procAttrs->paramList);
    freeScope(obj->procAttrs->scope);
    free(obj->procAttrs);
    break;
  case OBJ_PROGRAM:
    freeScope(obj->progAttrs->scope);
    free(obj->progAttrs);
    break;
  case OBJ_PARAMETER:
    freeType(obj->paramAttrs->type);
    free(obj->paramAttrs);
  }
  free(obj);
}

/* 
 * HÀM freeScope: Giải phóng bộ nhớ của scope
 * 
 * THAM SỐ:
 *   - scope: con trỏ đến scope cần giải phóng
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Giải phóng tất cả object trong scope (danh sách objList)
 *   2. Giải phóng bộ nhớ scope chính
 * 
 * MỤC ĐÍCH:
 *   - Dọn dẹp scope khi thoát khỏi block
 */
void freeScope(Scope* scope) {
  freeObjectList(scope->objList);
  free(scope);
}

/* 
 * HÀM freeObjectList: Giải phóng danh sách linked list của object
 * 
 * THAM SỐ:
 *   - objList: con trỏ đến phần tử đầu của danh sách
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Duyệt qua từng ObjectNode trong danh sách
 *   2. Lưu con trỏ tiếp theo trước khi giải phóng (tránh truy cập memory đã free)
 *   3. Giải phóng object và node
 *   4. Chuyển đến phần tử tiếp theo
 * 
 * GHI CHÚ:
 *   - ObjectNode chứa object, next (con trỏ đến phần tử tiếp theo)
 * 
 * VÍ DỤ:
 *   // Giải phóng tất cả object trong scope
 *   freeObjectList(scope->objList);
 */
void freeObjectList(ObjectNode *objList) {
  ObjectNode* list = objList;

  while (list != NULL) {
    ObjectNode* node = list;
    list = list->next;
    freeObject(node->object);  /* Giải phóng object bên trong */
    free(node);
  }
}

/* 
 * HÀM freeReferenceList: Giải phóng danh sách tham chiếu object
 * 
 * THAM SỐ:
 *   - objList: con trỏ đến danh sách tham chiếu
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Duyệt qua từng ObjectNode trong danh sách
 *   2. Giải phóng node (không giải phóng object, chỉ là tham chiếu)
 * 
 * MỤC ĐÍCH:
 *   - Khác với freeObjectList: chỉ giải phóng node, không giải phóng object
 *   - Dùng cho danh sách tham số (tham chiếu đến object khác, không sở hữu)
 * 
 * VÍ DỤ:
 *   // Giải phóng danh sách tham số (object param vẫn tồn tại ở đơn mục)
 *   freeReferenceList(func->paramList);
 */
void freeReferenceList(ObjectNode *objList) {
  ObjectNode* list = objList;

  while (list != NULL) {
    ObjectNode* node = list;
    list = list->next;
    free(node);  /* Chỉ giải phóng node, object được quản lý ở nơi khác */
  }
}

/* 
 * HÀM addObject: Thêm object vào danh sách object
 * 
 * THAM SỐ:
 *   - objList: con trỏ đến danh sách object (con trỏ kép)
 *   - obj: con trỏ đến object cần thêm
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Tạo ObjectNode mới
 *   2. Lưu object vào node
 *   3. Nếu danh sách trống: thêm ở đầu
 *   4. Nếu có object rồi: duyệt đến cuối, thêm vào cuối (FIFO)
 * 
 * MỤC ĐÍCH:
 *   - Thêm biến, hàm, const,... vào bảng ký hiệu
 *   - Quản lý danh sách object trong scope
 * 
 * GHI CHÚ:
 *   - Dùng con trỏ kép (&objList) để thay đổi danh sách gốc
 *   
 * VÍ DỤ:
 *   Object* x = createVariableObject("x");
 *   x->varAttrs->type = makeIntType();
 *   addObject(&(scope->objList), x);  // x đã được thêm vào
 */
void addObject(ObjectNode **objList, Object* obj) {
  ObjectNode* node = (ObjectNode*) malloc(sizeof(ObjectNode));
  node->object = obj;
  node->next = NULL;
  if ((*objList) == NULL) 
    *objList = node;  /* Danh sách trống, thêm ở đầu */
  else {
    ObjectNode *n = *objList;
    while (n->next != NULL) 
      n = n->next;  /* Duyệt đến cuối */
    n->next = node;  /* Thêm ở cuối */
  }
}

/* 
 * HÀM findObject: Tìm object trong danh sách theo tên
 * 
 * THAM SỐ:
 *   - objList: con trỏ đến danh sách object cần tìm
 *   - name: tên object cần tìm
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến object nếu tìm thấy, NULL nếu không
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Duyệt danh sách object
 *   2. So sánh tên từng object với tên cần tìm
 *   3. Trả về object đầu tiên có tên trùng khớp
 *   4. Trả về NULL nếu không tìm thấy
 * 
 * MỤC ĐÍCH:
 *   - Kiểm tra object đã tồn tại trong scope chưa
 *   - Tìm kiếm object theo tên
 * 
 * GHI CHÚ:
 *   - Chỉ tìm kiếm trong danh sách hiện tại, không tìm parent scope
 *   - Dùng strcmp để so sánh chuỗi
 * 
 * VÍ DỤ:
 *   // Kiểm tra x đã được khai báo chưa
 *   if (findObject(objList, "x") != NULL) {
 *     error(SYMBOL_REDECLARED, ...);  // Lỗi khai báo lại
 *   }
 */
Object* findObject(ObjectNode *objList, char *name) {
  while (objList != NULL) {
    if (strcmp(objList->object->name, name) == 0) 
      return objList->object;  /* Tìm thấy, trả về */
    else objList = objList->next;  /* Chuyển sang object tiếp theo */
  }
  return NULL;  /* Không tìm thấy */
}

/******* CÁC HÀM KHỞI TẠO VÀ DỌN DẸP BẢNG KÝ HIỆU *******/

/* 
 * HÀM initSymTab: Khởi tạo bảng ký hiệu (symbol table)
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Cấp phát bộ nhớ cho bảng ký hiệu
 *   2. Khởi tạo globalObjectList = NULL (danh sách object toàn cục)
 *   3. Tạo 5 hàm/thủ tục chuẩn (built-in):
 *      a) READC: đọc ký tự (trả về char)
 *      b) READI: đọc số nguyên (trả về int)
 *      c) WRITEI: in số nguyên (tham số i: int)
 *      d) WRITEC: in ký tự (tham số ch: char)
 *      e) WRITELN: in dòng mới (không tham số)
 *   4. Khởi tạo các kiểu chuẩn: intType, charType
 * 
 * MỤC ĐÍCH:
 *   - Chuẩn bị bảng ký hiệu trước khi biên dịch
 *   - Cung cấp các hàm nhập/xuất chuẩn cho chương trình
 * 
 * GHI CHÚ:
 *   - Phải gọi initSymTab() trước khi bắt đầu biên dịch
 *   - Phải gọi cleanSymTab() để dọn dẹp sau khi hoàn thành
 */
void initSymTab(void) {
  Object* obj;
  Object* param;

  symtab = (SymTab*) malloc(sizeof(SymTab));
  symtab->globalObjectList = NULL;  /* Danh sách object toàn cục ban đầu trống */
  
  /* Hàm READC: đọc ký tự */
  obj = createFunctionObject("READC");
  obj->funcAttrs->returnType = makeCharType();
  addObject(&(symtab->globalObjectList), obj);

  /* Hàm READI: đọc số nguyên */
  obj = createFunctionObject("READI");
  obj->funcAttrs->returnType = makeIntType();
  addObject(&(symtab->globalObjectList), obj);

  /* Thủ tục WRITEI: in số nguyên, tham số i: int */
  obj = createProcedureObject("WRITEI");
  param = createParameterObject("i", PARAM_VALUE, obj);
  param->paramAttrs->type = makeIntType();
  addObject(&(obj->procAttrs->paramList), param);  /* Thêm tham số */
  addObject(&(symtab->globalObjectList), obj);

  /* Thủ tục WRITEC: in ký tự, tham số ch: char */
  obj = createProcedureObject("WRITEC");
  param = createParameterObject("ch", PARAM_VALUE, obj);
  param->paramAttrs->type = makeCharType();
  addObject(&(obj->procAttrs->paramList), param);  /* Thêm tham số */
  addObject(&(symtab->globalObjectList), obj);

  /* Thủ tục WRITELN: in dòng mới, không tham số */
  obj = createProcedureObject("WRITELN");
  addObject(&(symtab->globalObjectList), obj);

  /* Khởi tạo kiểu chuẩn */
  intType = makeIntType();
  charType = makeCharType();
}

/* 
 * HÀM cleanSymTab: Dọn dẹp bảng ký hiệu
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Giải phóng object chương trình (program)
 *   2. Giải phóng danh sách object toàn cục
 *   3. Giải phóng bảng ký hiệu
 *   4. Giải phóng các kiểu chuẩn (intType, charType)
 * 
 * MỤC ĐÍCH:
 *   - Tránh rò rỉ bộ nhớ
 *   - Dọn dẹp tất cả tài nguyên sau khi hoàn thành biên dịch
 * 
 * GHI CHÚ:
 *   - Phải gọi sau khi hoàn thành biên dịch
 *   - Gọi cleanSymTab() trước khi kết thúc chương trình
 */
void cleanSymTab(void) {
  freeObject(symtab->program);  /* Giải phóng object chương trình */
  freeObjectList(symtab->globalObjectList);  /* Giải phóng danh sách toàn cục */
  free(symtab);  /* Giải phóng bảng ký hiệu */
  freeType(intType);  /* Giải phóng kiểu int */
  freeType(charType);  /* Giải phóng kiểu char */
}

/******* CÁC HÀM QUẢN LÝ SCOPE (PHẠM VI) *******/

/* 
 * HÀM enterBlock: Vào block mới (scope mới)
 * 
 * THAM SỐ:
 *   - scope: con trỏ đến scope cần vào
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Đặt currentScope = scope
 *   2. Khi biên dịch block, tất cả variable sẽ được thêm vào scope này
 * 
 * MỤC ĐÍCH:
 *   - Chuyển context sang scope mới (khi vào block, hàm, thủ tục)
 *   - Cho phép khai báo variable cục bộ
 * 
 * GHI CHÚ:
 *   - Phải có exitBlock() tương ứng sau khi thoát block
 * 
 * VÍ DỤ:
 *   // Vào scope của hàm f
 *   enterBlock(f->funcAttrs->scope);
 *   // ... biên dịch hàm ...
 *   exitBlock();  // Thoát scope hàm
 */
void enterBlock(Scope* scope) {
  symtab->currentScope = scope;  /* Lưu scope hiện tại */
}

/* 
 * HÀM exitBlock: Thoát block (scope) hiện tại
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Trở về scope bên ngoài (outer scope)
 *   2. Gán currentScope = currentScope->outer
 * 
 * MỤC ĐÍCH:
 *   - Thoát khỏi scope (block, hàm, thủ tục)
 *   - Quay lại scope cha để tiếp tục biên dịch
 * 
 * GHI CHÚ:
 *   - Phải có enterBlock() trước đó
 *   - Nếu scope->outer = NULL, sẽ gây lỗi
 */
void exitBlock(void) {
  symtab->currentScope = symtab->currentScope->outer;  /* Quay lại scope cha */
}

/******* CÁC HÀM TÌM KIẾM OBJECT (NÉO SCOPE) *******/

/* 
 * HÀM lookupObject: Tìm object theo chuỗi scope
 * 
 * THAM SỐ:
 *   - name: tên object cần tìm
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến object nếu tìm thấy, NULL nếu không
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Bắt đầu từ currentScope (scope hiện tại)
 *   2. Tìm object trong scope hiện tại (gọi findObject)
 *   3. Nếu tìm thấy: return object
 *   4. Nếu không: chuyển lên scope cha (outer scope)
 *   5. Lặp lại đến khi tìm thấy hoặc outer = NULL
 * 
 * MỤC ĐÍCH:
 *   - Tìm kiếm object trong chuỗi scope (scope chain)
 *   - Hỗ trợ quy tắc scoping: tìm innermost đến outermost
 * 
 * VÍ DỤ:
 *   // Tìm biến x có thể ở scope hiện tại hoặc scope cha
 *   Object* x = lookupObject("x");
 *   if (x == NULL) 
 *     error(ERR_UNDECLARED, ...);
 */
Object* lookupObject(char *name) {
  Scope* scope = symtab->currentScope;
  
  while (scope != NULL) {
    Object* obj = findObject(scope->objList, name);
    if (obj != NULL)
      return obj;  /* Tìm thấy */
    scope = scope->outer;  /* Chuyển lên scope cha */
  }
  
  return NULL;  /* Không tìm thấy trong chuỗi scope */
}

/******* CÁC HÀM KIỂM TRA KHAI BÁO *******/

/* 
 * HÀM checkFreshIdent: Kiểm tra identifier chưa được khai báo
 * 
 * THAM SỐ:
 *   - name: tên identifier cần kiểm tra
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Tìm object trong scope hiện tại (không tìm parent scope)
 *   2. Nếu tìm thấy: báo lỗi ERR_DUPLICATE_IDENT
 * 
 * MỤC ĐÍCH:
 *   - Đảm bảo không khai báo lại identifier trong cùng scope
 *   - Sử dụng trước khi tạo object mới
 * 
 * GHI CHÚ:
 *   - Chỉ kiểm tra scope hiện tại, không kiểm tra parent scope
 *   - Dùng khi khai báo variable, const, type, function,...
 * 
 * VÍ DỤ:
 *   // Kiểm tra trước khi khai báo biến x
 *   checkFreshIdent("x");  // Nếu x đã có → báo lỗi
 *   Object* x = createVariableObject("x");
 */
void checkFreshIdent(char *name) {
  if (findObject(symtab->currentScope->objList, name) != NULL) {
    error(ERR_DUPLICATE_IDENT, currentToken->lineNo, currentToken->colNo);
  }
}

/* 
 * HÀM checkDeclaredConstant: Kiểm tra hằng số đã được khai báo
 * 
 * THAM SỐ:
 *   - name: tên hằng cần kiểm tra
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến object hằng nếu hợp lệ, NULL nếu lỗi
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Tìm object trong chuỗi scope (gọi lookupObject)
 *   2. Nếu không tìm thấy: báo lỗi ERR_UNDECLARED_IDENT, return NULL
 *   3. Nếu tìm thấy nhưng kind != OBJ_CONSTANT: báo lỗi ERR_INVALID_CONSTANT
 *   4. Nếu OK: return object
 * 
 * MỤC ĐÍCH:
 *   - Kiểm tra constant khi sử dụng trong expression
 *   - Đảm bảo object là constant, không phải variable hay loại khác
 * 
 * VÍ DỤ:
 *   // Kiểm tra MAX là constant khi sử dụng
 *   Object* maxConst = checkDeclaredConstant("MAX");
 *   if (maxConst != NULL)
 *     printf("Value: %d", maxConst->constAttrs->value->intValue);
 */
Object* checkDeclaredConstant(char *name) {
  Object *obj = lookupObject(name);
  if (obj == NULL) {
    error(ERR_UNDECLARED_IDENT, currentToken->lineNo, currentToken->colNo);
    return NULL;
  }
  if (obj->kind != OBJ_CONSTANT) {
    error(ERR_INVALID_CONSTANT, currentToken->lineNo, currentToken->colNo);
    return NULL;
  }
  return obj;
}

/* 
 * HÀM checkDeclaredType: Kiểm tra kiểu đã được khai báo
 * 
 * THAM SỐ:
 *   - name: tên kiểu cần kiểm tra
 * 
 * TRẢ VỀ:
 *   - Con trỏ đến object kiểu nếu hợp lệ, NULL nếu lỗi
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Tìm object trong chuỗi scope (gọi lookupObject)
 *   2. Nếu không tìm thấy: báo lỗi ERR_UNDECLARED_IDENT, return NULL
 *   3. Nếu tìm thấy nhưng kind != OBJ_TYPE: báo lỗi ERR_INVALID_TYPE
 *   4. Nếu OK: return object
 * 
 * MỤC ĐÍCH:
 *   - Kiểm tra kiểu khi sử dụng trong khai báo (biến, tham số,...)
 *   - Đảm bảo object là TYPE, không phải constant hay loại khác
 * 
 * VÍ DỤ:
 *   // Kiểm tra ArrayType đã được định nghĩa
 *   Object* arrType = checkDeclaredType("ArrayType");
 *   // Sau đó dùng arrType->typeAttrs->actualType
 */
Object* checkDeclaredType(char *name) {
  Object *obj = lookupObject(name);
  if (obj == NULL) {
    error(ERR_UNDECLARED_IDENT, currentToken->lineNo, currentToken->colNo);
    return NULL;
  }
  if (obj->kind != OBJ_TYPE) {
    error(ERR_INVALID_TYPE, currentToken->lineNo, currentToken->colNo);
    return NULL;
  }
  return obj;
}

/* 
 * HÀM declareObject: Khai báo object (thêm vào bảng ký hiệu)
 * 
 * THAM SỐ:
 *   - obj: con trỏ đến object cần khai báo
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Nếu obj->kind == OBJ_PARAMETER:
 *      - Tìm owner (hàm/thủ tục chứa parameter)
 *      - Thêm parameter vào paramList của owner
 *   2. Thêm object vào currentScope->objList
 * 
 * MỤC ĐÍCH:
 *   - Khai báo object vào bảng ký hiệu
 *   - Xử lý đặc biệt cho tham số (thêm vào 2 nơi: scope + paramList)
 * 
 * VÍ DỤ:
 *   // Khai báo biến x
 *   Object* x = createVariableObject("x");
 *   x->varAttrs->type = makeIntType();
 *   declareObject(x);  // x đã được khai báo
 *   
 *   // Khai báo tham số
 *   Object* param = createParameterObject("i", PARAM_VALUE, func);
 *   param->paramAttrs->type = makeIntType();
 *   declareObject(param);  // Thêm vào cả paramList và currentScope
 */
void declareObject(Object* obj) {
  if (obj->kind == OBJ_PARAMETER) {
    /* Nếu là parameter, thêm vào danh sách tham số của owner */
    Object* owner = symtab->currentScope->owner;
    switch (owner->kind) {
    case OBJ_FUNCTION:
      addObject(&(owner->funcAttrs->paramList), obj);
      break;
    case OBJ_PROCEDURE:
      addObject(&(owner->procAttrs->paramList), obj);
      break;
    default:
      break;
    }
  }
 
  /* Thêm vào danh sách object của scope hiện tại */
  addObject(&(symtab->currentScope->objList), obj);
}


