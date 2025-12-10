#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "error.h"

void freeObject(Object* obj);
void freeScope(Scope* scope);
void freeObjectList(ObjectNode *objList);
void freeReferenceList(ObjectNode *objList);

SymTab* symtab;
Type* intType;
Type* charType;

/************************ CÁC HÀM TIỆN ÍCH VỀ KIỂU DỮ LIỆU ******************************/

/* 
 * Hàm makeIntType: Tạo kiểu dữ liệu Int
 * 
 * Mục đích:
 *   - Cấp phát bộ nhớ cho một cấu trúc Type
 *   - Khởi tạo typeClass = TP_INT
 * 
 * Trả về:
 *   - Con trỏ đến Type mới tạo
 * 
 * Ví dụ:
 *   Type* intType = makeIntType();
 *   // intType->typeClass = TP_INT
 */
Type* makeIntType(void) {
    Type* type = (Type*) malloc(sizeof(Type));
    type->typeClass = TP_INT;
    return type;
}

/* 
 * Hàm makeCharType: Tạo kiểu dữ liệu Char
 * 
 * Mục đích:
 *   - Cấp phát bộ nhớ cho một cấu trúc Type
 *   - Khởi tạo typeClass = TP_CHAR
 * 
 * Trả về:
 *   - Con trỏ đến Type mới tạo
 */
Type* makeCharType(void) {
    Type* type = (Type*) malloc(sizeof(Type));
    type->typeClass = TP_CHAR;
    return type;
}

/* 
 * Hàm makeArrayType: Tạo kiểu dữ liệu mảng
 * 
 * Mục đích:
 *   - Tạo kiểu mảng với kích thước và kiểu phần tử cho trước
 *   - Hỗ trợ mảng lồng nhau (multidimensional array)
 * 
 * Tham số:
 *   - arraySize: kích thước mảng (ví dụ: 10)
 *   - elementType: con trỏ đến Type của phần tử (có thể là Int, Char, hoặc Array khác)
 * 
 * Trả về:
 *   - Con trỏ đến Type mảng mới tạo
 * 
 * Ví dụ:
 *   Type* arr = makeArrayType(10, makeIntType());  // Mảng 10 phần tử Int
 *   Type* arr2d = makeArrayType(10, makeArrayType(5, makeIntType()));  // Mảng 10x5
 */
Type* makeArrayType(int arraySize, Type* elementType) {
    Type* type = (Type*) malloc(sizeof(Type));
    type->typeClass = TP_ARRAY;
    type->arraySize = arraySize;
    type->elementType = elementType;  // Lưu con trỏ đến kiểu phần tử
    return type;
}

/* 
 * Hàm duplicateType: Sao chép (copy) một Type
 * 
 * Mục đích:
 *   - Tạo một bản sao độc lập của Type (deep copy)
 *   - Dùng khi cần giữ lại bản gốc mà vẫn có một bản riêng
 *   - Xử lý đệ quy cho mảng lồng nhau
 * 
 * Tham số:
 *   - type: con trỏ đến Type cần sao chép
 * 
 * Trả về:
 *   - Con trỏ đến Type sao chép (mới cấp phát bộ nhớ)
 * 
 * Ví dụ:
 *   Type* original = makeArrayType(10, makeIntType());
 *   Type* copy = duplicateType(original);  // Bản sao độc lập
 */
Type* duplicateType(Type* type) {
    Type* resultType = (Type*) malloc(sizeof(Type));
    resultType->typeClass = type->typeClass;
    if (type->typeClass == TP_ARRAY) {
        resultType->arraySize = type->arraySize;
        // Gọi đệ quy để sao chép elementType (trường hợp mảng lồng nhau)
        resultType->elementType = duplicateType(type->elementType);
    }
    return resultType;
}

/* 
 * Hàm compareType: So sánh hai Type
 * 
 * Mục đích:
 *   - Kiểm tra xem hai Type có giống nhau không
 *   - Dùng để kiểm tra kiểu trong khi parse/compile
 * 
 * Tham số:
 *   - type1, type2: hai con trỏ Type cần so sánh
 * 
 * Trả về:
 *   - 1 nếu hai Type giống nhau
 *   - 0 nếu khác nhau
 * 
 * Cách hoạt động:
 *   - So sánh typeClass trước
 *   - Nếu là mảng, cũng kiểm tra arraySize và elementType (đệ quy)
 * 
 * Ví dụ:
 *   Type* arr1 = makeArrayType(10, makeIntType());
 *   Type* arr2 = makeArrayType(10, makeIntType());
 *   compareType(arr1, arr2);  // Trả về 1 (giống nhau)
 */
int compareType(Type* type1, Type* type2) {
    if (type1->typeClass == type2->typeClass) {
        if (type1->typeClass == TP_ARRAY) {
            // Kiểm tra kích thước mảng
            if (type1->arraySize == type2->arraySize)
                // Gọi đệ quy để so sánh elementType
                return compareType(type1->elementType, type2->elementType);
            else return 0;
        } else return 1;  // Int hoặc Char giống nhau
    } else return 0;
}

/* 
 * Hàm freeType: Giải phóng bộ nhớ của Type
 * 
 * Mục đích:
 *   - Xóa Type và các Type lồng nhau (trong trường hợp mảng)
 *   - Tránh rò rỉ bộ nhớ (memory leak)
 * 
 * Tham số:
 *   - type: con trỏ đến Type cần giải phóng
 * 
 * Cách hoạt động:
 *   - Nếu là TP_INT hoặc TP_CHAR: chỉ cần free Type đó
 *   - Nếu là TP_ARRAY: gọi đệ quy để free elementType trước
 * 
 * Ví dụ:
 *   Type* arr = makeArrayType(10, makeIntType());
 *   freeType(arr);  // Giải phóng cả arr và elementType
 */
void freeType(Type* type) {
    switch (type->typeClass) {
        case TP_INT:
        case TP_CHAR:
            free(type);
            break;
        case TP_ARRAY:
            // Gọi đệ quy để free elementType trước (bottom-up)
            freeType(type->elementType);
            free(type);
            break;
    }
}

/************************ CÁC HÀM TIỆN ÍCH VỀ GIÁ TRỊ HẰNG SỐ ******************************/

/* 
 * Hàm makeIntConstant: Tạo giá trị hằng số kiểu Int
 * 
 * Mục đích:
 *   - Cấp phát bộ nhớ và khởi tạo ConstantValue cho số nguyên
 * 
 * Tham số:
 *   - i: giá trị số nguyên (ví dụ: 10, 42, 100)
 * 
 * Trả về:
 *   - Con trỏ đến ConstantValue mới tạo với intValue = i
 * 
 * Ví dụ:
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
 * Hàm makeCharConstant: Tạo giá trị hằng số kiểu Char
 * 
 * Mục đích:
 *   - Cấp phát bộ nhớ và khởi tạo ConstantValue cho ký tự
 * 
 * Tham số:
 *   - ch: giá trị ký tự (ví dụ: 'a', 'Z', '0')
 * 
 * Trả về:
 *   - Con trỏ đến ConstantValue mới tạo với charValue = ch
 * 
 * Ví dụ:
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
 * Hàm duplicateConstantValue: Sao chép giá trị hằng số
 * 
 * Mục đích:
 *   - Tạo bản sao độc lập của ConstantValue
 *   - Dùng khi cần giữ nguyên bản gốc
 * 
 * Tham số:
 *   - v: con trỏ đến ConstantValue cần sao chép
 * 
 * Trả về:
 *   - Con trỏ đến ConstantValue sao chép (mới cấp phát bộ nhớ)
 * 
 * Ví dụ:
 *   ConstantValue* original = makeIntConstant(10);
 *   ConstantValue* copy = duplicateConstantValue(original);
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

/************************ CÁC HÀM TIỆN ÍCH VỀ PHẠM VI (SCOPE) VÀ ĐỐI TƯỢNG ******************************/

/* 
 * Hàm createScope: Tạo một phạm vi (scope) mới
 * 
 * Mục đích:
 *   - Cấp phát bộ nhớ cho Scope
 *   - Khởi tạo quan hệ phạm vi (nested scope)
 * 
 * Tham số:
 *   - owner: con trỏ đến Object sở hữu scope (hàm/thủ tục/chương trình)
 *   - outer: con trỏ đến scope bên ngoài (parent scope), NULL nếu là scope toàn cục
 * 
 * Trả về:
 *   - Con trỏ đến Scope mới tạo
 * 
 * Ví dụ:
 *   // Tạo scope cho chương trình (không có outer scope)
 *   Scope* progScope = createScope(program, NULL);
 *   
 *   // Tạo scope cục bộ cho hàm (outer = chương trình)
 *   Scope* funcScope = createScope(function, progScope);
 */
Scope* createScope(Object* owner, Scope* outer) {
    Scope* scope = (Scope*) malloc(sizeof(Scope));
    scope->objList = NULL;  // Danh sách object ban đầu trống
    scope->owner = owner;   // Ai sở hữu scope này
    scope->outer = outer;   // Con trỏ đến scope bên ngoài (để tìm kiếm ký hiệu)
    return scope;
}

/* 
 * Hàm createProgramObject: Tạo đối tượng chương trình
 * 
 * Mục đích:
 *   - Tạo object đại diện cho toàn bộ chương trình
 *   - Khởi tạo scope toàn cục cho chương trình
 *   - Đây là object gốc của bảng ký hiệu
 * 
 * Tham số:
 *   - programName: tên của chương trình (ví dụ: "PRG")
 * 
 * Trả về:
 *   - Con trỏ đến Object chương trình mới tạo
 * 
 * Ví dụ:
 *   Object* program = createProgramObject("PRG");
 *   // Tạo scope toàn cục với owner = program, outer = NULL
 */
Object* createProgramObject(char *programName) {
    Object* program = (Object*) malloc(sizeof(Object));
    strcpy(program->name, programName);
    program->kind = OBJ_PROGRAM;
    program->progAttrs = (ProgramAttributes*) malloc(sizeof(ProgramAttributes));
    program->progAttrs->scope = createScope(program, NULL);  // Scope toàn cục
    symtab->program = program;  // Ghi vào bảng ký hiệu toàn cục

    return program;
}

/* 
 * Hàm createConstantObject: Tạo đối tượng hằng số
 * 
 * Mục đích:
 *   - Tạo object đại diện cho một hằng số
 *   - Cấp phát bộ nhớ cho constAttrs (sẽ gán giá trị sau)
 * 
 * Tham số:
 *   - name: tên của hằng số (ví dụ: "c1", "PI")
 * 
 * Trả về:
 *   - Con trỏ đến Object hằng số mới tạo
 * 
 * Ghi chú:
 *   - Sau khi tạo, cần gán giá trị: obj->constAttrs->value = makeIntConstant(10);
 */
Object* createConstantObject(char *name) {
    Object* obj = (Object*) malloc(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = OBJ_CONSTANT;
    obj->constAttrs = (ConstantAttributes*) malloc(sizeof(ConstantAttributes));
    return obj;
}

/* 
 * Hàm createTypeObject: Tạo đối tượng kiểu dữ liệu
 * 
 * Mục đích:
 *   - Tạo object đại diện cho một kiểu dữ liệu tự định nghĩa
 * 
 * Tham số:
 *   - name: tên của kiểu (ví dụ: "t1", "IntArray")
 * 
 * Trả về:
 *   - Con trỏ đến Object kiểu mới tạo
 * 
 * Ghi chú:
 *   - Sau khi tạo, cần gán kiểu thực tế: obj->typeAttrs->actualType = makeIntType();
 */
Object* createTypeObject(char *name) {
    Object* obj = (Object*) malloc(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = OBJ_TYPE;
    obj->typeAttrs = (TypeAttributes*) malloc(sizeof(TypeAttributes));
    return obj;
}

/* 
 * Hàm createVariableObject: Tạo đối tượng biến
 * 
 * Mục đích:
 *   - Tạo object đại diện cho một biến
 *   - Ghi lại scope mà biến được khai báo
 * 
 * Tham số:
 *   - name: tên của biến (ví dụ: "v1", "count")
 * 
 * Trả về:
 *   - Con trỏ đến Object biến mới tạo
 * 
 * Ghi chú:
 *   - Sau khi tạo, cần gán kiểu: obj->varAttrs->type = makeIntType();
 *   - scope được ghi lại tự động từ currentScope
 */
Object* createVariableObject(char *name) {
    Object* obj = (Object*) malloc(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = OBJ_VARIABLE;
    obj->varAttrs = (VariableAttributes*) malloc(sizeof(VariableAttributes));
    obj->varAttrs->scope = symtab->currentScope;  // Lưu lại scope hiện tại
    return obj;
}

/* 
 * Hàm createFunctionObject: Tạo đối tượng hàm
 * 
 * Mục đích:
 *   - Tạo object đại diện cho một hàm
 *   - Khởi tạo scope cục bộ cho hàm (lồng trong current scope)
 * 
 * Tham số:
 *   - name: tên của hàm (ví dụ: "f", "calculate")
 * 
 * Trả về:
 *   - Con trỏ đến Object hàm mới tạo
 * 
 * Ghi chú:
 *   - Scope của hàm là con của currentScope (để quản lý biến cục bộ)
 *   - Sau khi tạo, cần gán kiểu trả về: obj->funcAttrs->returnType = makeIntType();
 *   - Tham số sẽ được thêm vào paramList bằng declareObject()
 */
Object* createFunctionObject(char *name) {
    Object* obj = (Object*) malloc(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = OBJ_FUNCTION;
    obj->funcAttrs = (FunctionAttributes*) malloc(sizeof(FunctionAttributes));
    obj->funcAttrs->paramList = NULL;  // Danh sách tham số ban đầu trống
    obj->funcAttrs->scope = createScope(obj, symtab->currentScope);  // Scope lồng
    return obj;
}

/* 
 * Hàm createProcedureObject: Tạo đối tượng thủ tục
 * 
 * Mục đích:
 *   - Tạo object đại diện cho một thủ tục
 *   - Khởi tạo scope cục bộ cho thủ tục (lồng trong current scope)
 * 
 * Tham số:
 *   - name: tên của thủ tục (ví dụ: "p", "print")
 * 
 * Trả về:
 *   - Con trỏ đến Object thủ tục mới tạo
 * 
 * Ghi chú:
 *   - Thủ tục giống hàm nhưng không có kiểu trả về
 *   - Scope của thủ tục là con của currentScope
 *   - Tham số sẽ được thêm vào paramList bằng declareObject()
 */
Object* createProcedureObject(char *name) {
    Object* obj = (Object*) malloc(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = OBJ_PROCEDURE;
    obj->procAttrs = (ProcedureAttributes*) malloc(sizeof(ProcedureAttributes));
    obj->procAttrs->paramList = NULL;  // Danh sách tham số ban đầu trống
    obj->procAttrs->scope = createScope(obj, symtab->currentScope);  // Scope lồng
    return obj;
}

/* 
 * Hàm createParameterObject: Tạo đối tượng tham số
 * 
 * Mục đích:
 *   - Tạo object đại diện cho một tham số của hàm/thủ tục
 *   - Lưu lại loại truyền tham số (value hay reference)
 * 
 * Tham số:
 *   - name: tên của tham số (ví dụ: "p1", "x")
 *   - kind: loại truyền tham số (PARAM_VALUE hoặc PARAM_REFERENCE)
 *   - owner: con trỏ đến hàm/thủ tục chứa tham số này
 * 
 * Trả về:
 *   - Con trỏ đến Object tham số mới tạo
 * 
 * Ghi chú:
 *   - Sau khi tạo, cần gán kiểu: obj->paramAttrs->type = makeIntType();
 */
Object* createParameterObject(char *name, enum ParamKind kind, Object* owner) {
    Object* obj = (Object*) malloc(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = OBJ_PARAMETER;
    obj->paramAttrs = (ParameterAttributes*) malloc(sizeof(ParameterAttributes));
    obj->paramAttrs->kind = kind;  // Loại truyền tham số
    obj->paramAttrs->function = owner;  // Hàm/thủ tục chứa tham số này
    return obj;
}

/* 
 * Hàm freeObject: Giải phóng bộ nhớ của một Object
 * 
 * Mục đích:
 *   - Xóa object và các dữ liệu liên quan
 *   - Tránh rò rỉ bộ nhớ (memory leak)
 * 
 * Tham số:
 *   - obj: con trỏ đến Object cần xóa
 * 
 * Cách hoạt động:
 *   - Tùy theo kind của object, giải phóng các thuộc tính riêng
 *   - OBJ_CONSTANT: xóa constAttrs->value và constAttrs
 *   - OBJ_TYPE: xóa typeAttrs->actualType và typeAttrs
 *   - OBJ_VARIABLE: xóa varAttrs->type và varAttrs
 *   - OBJ_FUNCTION/PROCEDURE: xóa paramList, scope, và các thuộc tính
 *   - OBJ_PROGRAM: xóa scope toàn cục
 *   - OBJ_PARAMETER: xóa paramAttrs->type và paramAttrs
 */
void freeObject(Object* obj) {
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
            freeReferenceList(obj->funcAttrs->paramList);  // Xóa paramList
            freeType(obj->funcAttrs->returnType);          // Xóa return type
            freeScope(obj->funcAttrs->scope);              // Xóa scope cục bộ
            free(obj->funcAttrs);
            break;
        case OBJ_PROCEDURE:
            freeReferenceList(obj->procAttrs->paramList);  // Xóa paramList
            freeScope(obj->procAttrs->scope);              // Xóa scope cục bộ
            free(obj->procAttrs);
            break;
        case OBJ_PROGRAM:
            freeScope(obj->progAttrs->scope);              // Xóa scope toàn cục
            free(obj->progAttrs);
            break;
        case OBJ_PARAMETER:
            freeType(obj->paramAttrs->type);
            free(obj->paramAttrs);
    }
    free(obj);  // Cuối cùng xóa object
}

/* 
 * Hàm freeScope: Giải phóng bộ nhớ của một Scope
 * 
 * Mục đích:
 *   - Xóa scope và tất cả object trong scope đó
 * 
 * Tham số:
 *   - scope: con trỏ đến Scope cần xóa
 */
void freeScope(Scope* scope) {
    freeObjectList(scope->objList);  // Xóa tất cả object trong scope
    free(scope);
}

/* 
 * Hàm freeObjectList: Giải phóng danh sách liên kết các Object
 * 
 * Mục đích:
 *   - Xóa tất cả node trong danh sách liên kết
 *   - Mỗi node chứa một object
 * 
 * Tham số:
 *   - objList: con trỏ đến node đầu tiên của danh sách
 * 
 * Cách hoạt động:
 *   - Duyệt danh sách từ đầu đến cuối
 *   - Mỗi bước: lưu node hiện tại, di chuyển pointer, xóa object và node
 */
void freeObjectList(ObjectNode *objList) {
    ObjectNode* list = objList;

    while (list != NULL) {
        ObjectNode* node = list;
        list = list->next;  // Di chuyển pointer trước khi xóa
        freeObject(node->object);  // Xóa object
        free(node);  // Xóa node
    }
}

/* 
 * Hàm freeReferenceList: Giải phóng danh sách liên kết (chỉ node)
 * 
 * Mục đích:
 *   - Xóa danh sách node mà không xóa object
 *   - Dùng cho danh sách tham số (object được xóa ở chỗ khác)
 * 
 * Tham số:
 *   - objList: con trỏ đến node đầu tiên của danh sách
 * 
 * Ghi chú:
 *   - Khác freeObjectList ở chỗ không gọi freeObject()
 *   - Chỉ xóa node, còn object được quản lý ở nơi khác
 */
void freeReferenceList(ObjectNode *objList) {
    ObjectNode* list = objList;

    while (list != NULL) {
        ObjectNode* node = list;
        list = list->next;  // Di chuyển pointer trước khi xóa
        free(node);  // Chỉ xóa node, không xóa object
    }
}

/* 
 * Hàm addObject: Thêm Object vào cuối danh sách liên kết
 * 
 * Mục đích:
 *   - Thêm một object vào danh sách object của một scope
 *   - Đảm bảo danh sách được duy trì theo thứ tự khai báo
 * 
 * Tham số:
 *   - objList: con trỏ đến con trỏ node đầu (để có thể thay đổi)
 *   - obj: con trỏ đến Object cần thêm
 * 
 * Cách hoạt động:
 *   - Tạo node mới chứa object
 *   - Nếu danh sách trống: node mới là phần tử đầu
 *   - Nếu danh sách có phần tử: tìm node cuối và liên kết
 * 
 * Ghi chú:
 *   - Dùng con trỏ đến con trỏ (ObjectNode**) để thay đổi danh sách ngoài hàm
 */
void addObject(ObjectNode **objList, Object* obj) {
    ObjectNode* node = (ObjectNode*) malloc(sizeof(ObjectNode));
    node->object = obj;
    node->next = NULL;
    
    if ((*objList) == NULL) 
        // Danh sách trống: node mới là phần tử đầu
        *objList = node;
    else {
        // Danh sách không trống: tìm node cuối
        ObjectNode *n = *objList;
        while (n->next != NULL) 
            n = n->next;
        // Liên kết node mới vào cuối
        n->next = node;
    }
}

/* 
 * Hàm findObject: Tìm Object theo tên
 * 
 * Mục đích:
 *   - Tìm kiếm một object trong danh sách bằng tên
 *   - Dùng để kiểm tra xem ký hiệu có tồn tại hay không
 * 
 * Tham số:
 *   - objList: con trỏ đến danh sách object cần tìm
 *   - name: tên của object cần tìm
 * 
 * Trả về:
 *   - Con trỏ đến object nếu tìm thấy
 *   - NULL nếu không tìm thấy
 * 
 * Cách hoạt động:
 *   - Duyệt danh sách từ đầu đến cuối
 *   - So sánh tên của từng object với name cần tìm
 *   - Trả về object ngay khi tìm thấy (không tìm tiếp)
 */
Object* findObject(ObjectNode *objList, char *name) {
    while (objList != NULL) {
        if (strcmp(objList->object->name, name) == 0) 
            return objList->object;  // Tìm thấy
        else objList = objList->next;  // Tiếp tục tìm
    }
    return NULL;  // Không tìm thấy
}

/************************ CÁC HÀM QUẢN LÝ BẢNG KÝ HIỆU ******************************/

/* 
 * Hàm initSymTab: Khởi tạo bảng ký hiệu
 * 
 * Mục đích:
 *   - Cấp phát bộ nhớ cho bảng ký hiệu toàn cục
 *   - Khởi tạo các hàm/thủ tục thư viện chuẩn
 * 
 * Các hàm/thủ tục được tạo sẵn:
 *   - READC(): hàm đọc ký tự, trả về Char
 *   - READI(): hàm đọc số nguyên, trả về Int
 *   - WRITEI(i): thủ tục in số nguyên (tham số i: Int)
 *   - WRITEC(ch): thủ tục in ký tự (tham số ch: Char)
 *   - WRITELN(): thủ tục xuống dòng
 * 
 * Ghi chú:
 *   - Các hàm này là một phần của ngôn ngữ, không được khai báo trong chương trình
 *   - Được khởi tạo để sử dụng trong main
 */
void initSymTab(void) {
    Object* obj;
    Object* param;

    // Cấp phát bộ nhớ cho bảng ký hiệu toàn cục
    symtab = (SymTab*) malloc(sizeof(SymTab));
    symtab->globalObjectList = NULL;
    
    // === Hàm READC: đọc ký tự ===
    obj = createFunctionObject("READC");
    obj->funcAttrs->returnType = makeCharType();  // Trả về Char
    addObject(&(symtab->globalObjectList), obj);

    // === Hàm READI: đọc số nguyên ===
    obj = createFunctionObject("READI");
    obj->funcAttrs->returnType = makeIntType();   // Trả về Int
    addObject(&(symtab->globalObjectList), obj);

    // === Thủ tục WRITEI: in số nguyên ===
    obj = createProcedureObject("WRITEI");
    param = createParameterObject("i", PARAM_VALUE, obj);
    param->paramAttrs->type = makeIntType();      // Tham số i: Int
    addObject(&(obj->procAttrs->paramList), param);
    addObject(&(symtab->globalObjectList), obj);

    // === Thủ tục WRITEC: in ký tự ===
    obj = createProcedureObject("WRITEC");
    param = createParameterObject("ch", PARAM_VALUE, obj);
    param->paramAttrs->type = makeCharType();     // Tham số ch: Char
    addObject(&(obj->procAttrs->paramList), param);
    addObject(&(symtab->globalObjectList), obj);

    // === Thủ tục WRITELN: xuống dòng ===
    obj = createProcedureObject("WRITELN");
    addObject(&(symtab->globalObjectList), obj);

    // Khởi tạo các kiểu cơ bản
    intType = makeIntType();
    charType = makeCharType();
}

/* 
 * Hàm cleanSymTab: Giải phóng bảng ký hiệu
 * 
 * Mục đích:
 *   - Xóa toàn bộ dữ liệu của bảng ký hiệu
 *   - Tránh rò rỉ bộ nhớ khi kết thúc chương trình
 * 
 * Cách hoạt động:
 *   - Xóa object chương trình (bao gồm scope toàn cục)
 *   - Xóa danh sách hàm/thủ tục thư viện
 *   - Xóa bảng ký hiệu và các kiểu cơ bản
 */
void cleanSymTab(void) {
    freeObject(symtab->program);              // Xóa chương trình
    freeObjectList(symtab->globalObjectList); // Xóa hàm/thủ tục thư viện
    free(symtab);                             // Xóa bảng ký hiệu
    freeType(intType);                        // Xóa kiểu Int
    freeType(charType);                       // Xóa kiểu Char
}

/* 
 * Hàm enterBlock: Vào một scope mới
 * 
 * Mục đích:
 *   - Thay đổi scope hiện tại thành scope mới
 *   - Dùng khi vào block của hàm/thủ tục/chương trình
 * 
 * Tham số:
 *   - scope: con trỏ đến scope muốn vào
 * 
 * Ví dụ:
 *   Object* func = createFunctionObject("f");
 *   declareObject(func);
 *   enterBlock(func->funcAttrs->scope);  // Vào scope của hàm f
 *   // Từ đây trở đi, các khai báo sẽ thuộc scope của hàm f
 */
void enterBlock(Scope* scope) {
    symtab->currentScope = scope;  // Cập nhật scope hiện tại
}

/* 
 * Hàm exitBlock: Thoát khỏi scope hiện tại
 * 
 * Mục đích:
 *   - Quay lại scope bên ngoài (parent scope)
 *   - Dùng khi kết thúc block của hàm/thủ tục
 * 
 * Ví dụ:
 *   // ... khai báo các biến cục bộ trong hàm ...
 *   exitBlock();  // Quay lại scope của chương trình chính
 * 
 * Ghi chú:
 *   - Scope hiện tại phải có outer scope (không được null)
 */
void exitBlock(void) {
    symtab->currentScope = symtab->currentScope->outer;  // Quay lại scope bên ngoài
}

/* 
 * Hàm declareObject: Khai báo object trong scope hiện tại
 * 
 * Mục đích:
 *   - Thêm object vào bảng ký hiệu của scope hiện tại
 *   - Xử lý đặc biệt cho tham số (thêm vào cả paramList)
 * 
 * Tham số:
 *   - obj: con trỏ đến object cần khai báo
 * 
 * Cách hoạt động:
 *   - Nếu là tham số (OBJ_PARAMETER):
 *     + Lấy owner của scope hiện tại (hàm/thủ tục)
 *     + Thêm tham số vào paramList của hàm/thủ tục
 *   - Thêm object vào objList của scope hiện tại
 * 
 * Ví dụ:
 *   Object* var = createVariableObject("v1");
 *   var->varAttrs->type = makeIntType();
 *   declareObject(var);  // v1 được khai báo trong scope hiện tại
 */
void declareObject(Object* obj) {
    if (obj->kind == OBJ_PARAMETER) {
        // Nếu là tham số: thêm vào paramList của hàm/thủ tục
        Object* owner = symtab->currentScope->owner;  // Lấy hàm/thủ tục sở hữu scope
        switch (owner->kind) {
            case OBJ_FUNCTION:
                addObject(&(owner->funcAttrs->paramList), obj);  // Thêm vào paramList của hàm
                break;
            case OBJ_PROCEDURE:
                addObject(&(owner->procAttrs->paramList), obj);  // Thêm vào paramList của thủ tục
                break;
            default:
                break;
        }
    }
    
    // Thêm object vào objList của scope hiện tại
    addObject(&(symtab->currentScope->objList), obj);
}
