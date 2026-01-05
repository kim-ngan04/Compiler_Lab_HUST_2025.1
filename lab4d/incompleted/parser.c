/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * ============================================================================
 * MÔ TẢ FILE: parser.c
 * ============================================================================
 * File parser.c thực hiện phân tích cú pháp (syntax analysis/parsing) cho 
 * ngôn ngữ KPL. Đây là giai đoạn thứ ba của bộ dịch, tiếp theo sau scanner.
 * 
 * CHỨC NĂNG CHÍNH:
 * - Nhận dòng tokens từ scanner
 * - Kiểm tra xem các tokens có tuân theo quy tắc ngữ pháp không
 * - Xây dựng cấu trúc bảng ký hiệu (Symbol Table) từ các khai báo
 * - Gọi các hàm semantic analysis để kiểm tra tính nhất quán kiểu dữ liệu
 * - In ra danh sách các objects đã khai báo nếu không có lỗi
 * 
 * CẤU TRÚC CHƯƠNG TRÌNH KPL:
 * program = "PROGRAM" id ";" block "."
 * block = [const_decl] [type_decl] [var_decl] [subprogram_decl] statements
 * const_decl = "CONST" {id "=" constant ";"} +
 * type_decl = "TYPE" {id "=" type ";"} +
 * var_decl = "VAR" {id ":" type ";"} +
 * subprogram_decl = {procedure_decl | function_decl}
 * statements = compound_statement
 * statement = assignment | call | compound | if | while | for
 * 
 * BIẾN TOÀN CỤC:
 * - currentToken: Token hiện tại đã được xử lý
 * - lookAhead: Token tiếp theo (được nhìn trước để quyết định)
 * - intType, charType: Các kiểu dữ liệu cơ bản
 * - symtab: Bảng ký hiệu toàn cục
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"

Token *currentToken;  // Token hiện tại đã được tiêu thụ
Token *lookAhead;     // Token tiếp theo được nhìn trước (lookahead token)

extern Type* intType;  // Kiểu int được định nghĩa toàn cục
extern Type* charType; // Kiểu char được định nghĩa toàn cục
extern SymTab* symtab; // Bảng ký hiệu được định nghĩa toàn cục

/**
 * scan()
 * ------
 * Hàm cập nhật các tokens: currentToken <- lookAhead, lookAhead <- token mới
 * Giải phóng bộ nhớ của currentToken cũ trước khi cập nhật.
 * Mục đích: Tiến đến token tiếp theo trong dòng tokens
 */
void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

/**
 * eat(TokenType tokenType)
 * -------------------------
 * Hàm tiêu thụ (consume) một token của loại tokenType nếu lookAhead khớp.
 * Nếu khớp: gọi scan() để tiến đến token tiếp theo
 * Nếu không khớp: báo lỗi (missing token)
 * 
 * @param tokenType: Loại token mong đợi
 */
void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

/**
 * compileProgram()
 * ----------------
 * Hàm phân tích toàn bộ chương trình KPL.
 * Cấu trúc: PROGRAM <id> ; <block> .
 * 
 * Quy trình:
 * 1. Tiêu thụ từ khóa PROGRAM
 * 2. Tiêu thụ tên chương trình (identifier)
 * 3. Tạo program object và nhập vào scope toàn cục
 * 4. Tiêu thụ dấu chấm phẩy
 * 5. Phân tích block (khai báo + câu lệnh)
 * 6. Tiêu thụ dấu chấm (kết thúc chương trình)
 * 7. Thoát scope
 */
void compileProgram(void) {
  Object* program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  exitBlock();
}

/**
 * compileBlock()
 * ----------------
 * Hàm phân tích một block (khối lệnh).
 * Cấu trúc: [const_decl] [type_decl] [var_decl] [func_decl|proc_decl]* statements
 * 
 * Quy trình: Sử dụng kiến trúc 5 bước để xử lý tuần tự các loại khai báo
 * 1. Xử lý khai báo hằng số (CONST) - compileBlock
 * 2. Xử lý khai báo kiểu (TYPE) - compileBlock2
 * 3. Xử lý khai báo biến (VAR) - compileBlock3
 * 4. Xử lý khai báo hàm/procedure - compileBlock4
 * 5. Xử lý các câu lệnh - compileBlock5
 */
void compileBlock(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);

    do {
      eat(TK_IDENT);
      
      // Kiểm tra tên không bị trùng trong scope hiện tại
      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);
      
      eat(SB_EQ);
      // Phân tích giá trị hằng số (có thể có dấu +/-)
      constValue = compileConstant();
      
      constObj->constAttrs->value = constValue;
      // Thêm hằng số vào bảng ký hiệu
      declareObject(constObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);  // Lặp nếu còn hằng số

    compileBlock2();  // Tiếp tục phân tích các phần còn lại
  } 
  else compileBlock2();
}

/**
 * compileBlock2()
 * ----------------
 * Bước 2: Phân tích khai báo kiểu dữ liệu (TYPE declarations)
 * Cấu trúc: TYPE id = type ;
 * 
 * Xử lý các khai báo kiểu mới, có thể bao gồm kiểu mảng hoặc kiểu tùy chỉnh
 */
void compileBlock2(void) {
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);

    do {
      eat(TK_IDENT);
      
      // Kiểm tra tên kiểu không bị trùng
      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);
      
      eat(SB_EQ);
      // Phân tích định nghĩa kiểu (có thể là INTEGER, CHAR, hoặc ARRAY [...] OF ...)
      actualType = compileType();
      
      typeObj->typeAttrs->actualType = actualType;
      // Thêm kiểu vào bảng ký hiệu
      declareObject(typeObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);  // Lặp nếu còn kiểu

    compileBlock3();  // Tiếp tục phân tích
  } 
  else compileBlock3();
}

/**
 * compileBlock3()
 * ----------------
 * Bước 3: Phân tích khai báo biến (VAR declarations)
 * Cấu trúc: VAR id : type ;
 * 
 * Xử lý các khai báo biến cục bộ trong block
 */
void compileBlock3(void) {
  Object* varObj;
  Type* varType;

  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);

    do {
      eat(TK_IDENT);
      
      // Kiểm tra tên biến không bị trùng
      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);

      eat(SB_COLON);
      // Phân tích kiểu của biến (cơ bản hoặc mảng)
      varType = compileType();
      
      varObj->varAttrs->type = varType;
      // Thêm biến vào bảng ký hiệu
      declareObject(varObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);  // Lặp nếu còn biến

    compileBlock4();  // Tiếp tục phân tích
  } 
  else compileBlock4();
}

/**
 * compileBlock4()
 * ----------------
 * Bước 4: Phân tích khai báo hàm (FUNCTION) và procedure (PROCEDURE)
 */
void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

/**
 * compileBlock5()
 * ----------------
 * Bước 5: Phân tích các câu lệnh chương trình
 * BEGIN <statements> END
 * 
 * Đây là phần thân (body) của chương trình hoặc hàm/procedure
 */
void compileBlock5(void) {
  eat(KW_BEGIN);
  // Phân tích danh sách các câu lệnh (có thể lồng nhau)
  compileStatements();
  eat(KW_END);
}

/**
 * compileSubDecls()
 * ------------------
 * Hàm phân tích các khai báo hàm và procedure
 * Lặp lại cho đến khi không còn FUNCTION hoặc PROCEDURE
 * 
 * Quy trình:
 * 1. Lặp trong khi còn FUNCTION hoặc PROCEDURE
 * 2. Nếu FUNCTION: gọi compileFuncDecl()
 * 3. Nếu PROCEDURE: gọi compileProcDecl()
 * 
 * Lưu ý: Hàm và procedure có thể được khai báo lồng nhau
 */
void compileSubDecls(void) {
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();  // Phân tích khai báo hàm
    else compileProcDecl();  // Phân tích khai báo procedure
  }
}

/**
 * compileFuncDecl()
 * ------------------
 * Hàm phân tích khai báo hàm (function declaration)
 * Cấu trúc: FUNCTION <id> [(<params>)] : <basic_type> ; <block> ;
 * 
 * Quy trình:
 * 1. Tiêu thụ từ khóa FUNCTION
 * 2. Tiêu thụ tên hàm
 * 3. Tạo function object và khai báo trong scope hiện tại
 * 4. Nhập vào scope mới của hàm (để xử lý tham số và biến cục bộ)
 * 5. Phân tích danh sách tham số (nếu có)
 * 6. Phân tích kiểu trả về (phải là INTEGER hoặc CHAR)
 * 7. Phân tích block (body của hàm)
 * 8. Thoát scope của hàm
 */
void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  // Kiểm tra tên hàm không bị trùng trong scope hiện tại
  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  // Khai báo hàm trong scope hiện tại (scope ngoài)
  declareObject(funcObj);

  // Tạo scope mới cho hàm (chứa tham số và biến cục bộ)
  enterBlock(funcObj->funcAttrs->scope);
  
  // Phân tích danh sách tham số (có thể lệnh tham chiếu hoặc giá trị)
  compileParams();

  eat(SB_COLON);
  // Phân tích kiểu trả về (chỉ có thể là INTEGER hoặc CHAR)
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  // Phân tích body của hàm (có thể có khai báo và câu lệnh)
  compileBlock();
  eat(SB_SEMICOLON);

  // Thoát khỏi scope của hàm
  exitBlock();
}

/**
 * compileProcDecl()
 * -------------------
 * Hàm phân tích khai báo procedure (quy trình)
 * Cấu trúc: PROCEDURE <id> [(<params>)] ; <block> ;
 * 
 * Tương tự compileFuncDecl nhưng:
 * - Không có kiểu trả về
 * - Chỉ có danh sách tham số (có thể có lệnh tham chiếu)
 */
void compileProcDecl(void) {
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  // Kiểm tra tên procedure không bị trùng
  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  // Khai báo procedure trong scope hiện tại
  declareObject(procObj);

  // Tạo scope mới cho procedure
  enterBlock(procObj->procAttrs->scope);

  // Phân tích danh sách tham số
  compileParams();

  eat(SB_SEMICOLON);
  // Phân tích body của procedure
  compileBlock();
  eat(SB_SEMICOLON);

  // Thoát khỏi scope của procedure
  exitBlock();
}

/**
 * compileUnsignedConstant()
 * -------------------------
 * Hàm phân tích hằng số không dấu (unsigned constant)
 * Có thể là: số nguyên, ký tự, hoặc tên hằng đã khai báo
 * 
 * Trả về: ConstantValue* chứa giá trị của hằng số
 */
ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:  // Số nguyên trực tiếp
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:  // Tên hằng đã khai báo
    eat(TK_IDENT);
    // Kiểm tra tên có được khai báo là hằng không
    obj = checkDeclaredConstant(currentToken->string);
    // Lấy giá trị từ object hằng
    constValue = duplicateConstantValue(obj->constAttrs->value);
    break;
  case TK_CHAR:  // Ký tự
    eat(TK_CHAR);
    // Lấy ký tự đầu tiên
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

/**
 * compileConstant()
 * -----------------
 * Hàm phân tích hằng số có thể có dấu +/-
 * Cấu trúc: [+ | -] <constant2> hoặc <char>
 * 
 * Xử lý: 
 * - Nếu có dấu PLUS: gọi compileConstant2
 * - Nếu có dấu MINUS: gọi compileConstant2 rồi phủ định giá trị
 * - Nếu là ký tự: phân tích ký tự
 * - Nếu không: gọi compileConstant2
 */
ConstantValue* compileConstant(void) {
  ConstantValue* constValue;

  switch (lookAhead->tokenType) {
  case SB_PLUS:  // Dấu cộng
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:  // Dấu trừ
    eat(SB_MINUS);
    constValue = compileConstant2();
    // Phủ định giá trị nguyên
    constValue->intValue = - constValue->intValue;
    break;
  case TK_CHAR:  // Ký tự trực tiếp
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:  // Không có dấu
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

/**
 * compileConstant2()
 * ------------------
 * Hàm phân tích hằng số không dấu hoặc hằng số integer đã khai báo
 * Chỉ xử lý số nguyên, không xử lý ký tự
 * 
 * Trả về: ConstantValue* của số nguyên
 */
ConstantValue* compileConstant2(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:  // Số nguyên trực tiếp
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:  // Tên hằng đã khai báo
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    // Kiểm tra hằng số phải là kiểu INT
    if (obj->constAttrs->value->type == TP_INT)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    else
      error(ERR_UNDECLARED_INT_CONSTANT,currentToken->lineNo, currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

/**
 * compileType()
 * ---------------
 * Hàm phân tích kiểu dữ liệu
 * Có thể là: INTEGER, CHAR, ARRAY [...] OF <type>, hoặc tên kiểu đã khai báo
 * 
 * Trả về: Type* chứa thông tin về kiểu dữ liệu
 * 
 * Lưu ý: ARRAY của ARRAY được hỗ trợ (mảng lồng nhau)
 */
Type* compileType(void) {
  Type* type;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
  case KW_INTEGER:  // Kiểu INTEGER
    eat(KW_INTEGER);
    type =  makeIntType();
    break;
  case KW_CHAR:  // Kiểu CHAR
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  case KW_ARRAY:  // Kiểu mảng
    eat(KW_ARRAY);
    eat(SB_LSEL);  // [
    eat(TK_NUMBER);
    // Lấy kích thước mảng
    arraySize = currentToken->value;
    eat(SB_RSEL);  // ]
    eat(KW_OF);
    // Phân tích kiểu phần tử (có thể lồng nhau)
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:  // Tên kiểu đã khai báo
    eat(TK_IDENT);
    // Kiểm tra tên kiểu có được khai báo không
    obj = checkDeclaredType(currentToken->string);
    // Sao chép kiểu từ object kiểu
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

/**
 * compileBasicType()
 * -------------------
 * Hàm phân tích kiểu dữ liệu cơ bản (chỉ INTEGER hoặc CHAR)
 * Không phải kiểu mảng hoặc kiểu tự định nghĩa
 * 
 * Trả về: Type* (chỉ có thể là INT_TYPE hoặc CHAR_TYPE)
 * 
 * Được sử dụng cho: kiểu trả về của hàm, kiểu tham số, ...
 */
Type* compileBasicType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER:  // Kiểu INTEGER
    eat(KW_INTEGER); 
    type = makeIntType();
    break;
  case KW_CHAR:  // Kiểu CHAR
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

/**
 * compileParams()
 * ----------------
 * Hàm phân tích danh sách tham số của hàm/procedure
 * Cấu trúc: ( <param> [; <param>]* ) hoặc rỗng
 * 
 * Xử lý:
 * - Nếu có dấu mở ngoặc: phân tích danh sách tham số
 * - Nếu không: không có tham số
 * 
 * Tham số được ngăn cách bởi dấu ;
 */
void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {  // (
    eat(SB_LPAR);
    // Phân tích tham số đầu tiên
    compileParam();
    // Phân tích các tham số tiếp theo
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);  // )
  }
}

/**
 * compileParam()
 * ---------------
 * Hàm phân tích một tham số đơn
 * Cấu trúc: [VAR] <id> : <basic_type>
 * 
 * Xử lý:
 * - Nếu có VAR: tham số tham chiếu (reference parameter)
 * - Nếu không: tham số giá trị (value parameter)
 * 
 * Tham số phải có kiểu cơ bản (INTEGER hoặc CHAR)
 */
void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind;

  switch (lookAhead->tokenType) {
  case TK_IDENT:  // Tham số giá trị
    paramKind = PARAM_VALUE;
    break;
  case KW_VAR:  // Tham số tham chiếu
    eat(KW_VAR);
    paramKind = PARAM_REFERENCE;
    break;
  default:
    error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
    break;
  }

  eat(TK_IDENT);
  // Kiểm tra tên tham số không bị trùng trong scope hiện tại
  checkFreshIdent(currentToken->string);
  // Tạo parameter object
  param = createParameterObject(currentToken->string, paramKind, symtab->currentScope->owner);
  eat(SB_COLON);
  // Phân tích kiểu của tham số (cơ bản)
  type = compileBasicType();
  param->paramAttrs->type = type;
  // Thêm tham số vào bảng ký hiệu
  declareObject(param);
}

/**
 * compileStatements()
 * --------------------
 * Hàm phân tích danh sách các câu lệnh
 * Cấu trúc: <statement> [; <statement>]*
 * 
 * Các câu lệnh được ngăn cách bởi dấu ;
 */
void compileStatements(void) {
  // Phân tích câu lệnh đầu tiên
  compileStatement();
  // Phân tích các câu lệnh tiếp theo
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

/**
 * compileStatement()
 * -------------------
 * Hàm phân tích một câu lệnh
 * Có thể là: gán, gọi procedure, khối lệnh, if, while, for
 * 
 * Xử lý FOLLOW set: các token có thể đến sau câu lệnh rỗng
 */
void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT:  // Câu lệnh gán: id := expr
    compileAssignSt();
    break;
  case KW_CALL:  // Câu lệnh gọi: call proc(...)
    compileCallSt();
    break;
  case KW_BEGIN:  // Khối lệnh: begin ... end
    compileGroupSt();
    break;
  case KW_IF:  // Câu lệnh if: if condition then ...
    compileIfSt();
    break;
  case KW_WHILE:  // Câu lệnh while: while condition do ...
    compileWhileSt();
    break;
  case KW_FOR:  // Câu lệnh for: for id := expr to expr do ...
    compileForSt();
    break;
    // Câu lệnh rỗng - các token FOLLOW
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    // Không làm gì (câu lệnh rỗng)
    break;
    // Lỗi
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

/**
 * compileLValue()
 * ----------------
 * Hàm phân tích giá trị trái (left value) của câu lệnh gán
 * Có thể là: 
 * - Biến: id
 * - Phần tử mảng: id[expr] hoặc id[expr][expr]...
 * - Tham số: id
 * - Tên hàm (để gán giá trị trả về): id
 * 
 * Trả về: Type* của left value
 */
Type* compileLValue(void) {
  Object* var;
  Type* varType;

  eat(TK_IDENT);
  // Kiểm tra xem identifier có được khai báo là lvalue không
  // (biến, tham số, hoặc hàm hiện tại)
  var = checkDeclaredLValueIdent(currentToken->string);
  
  if (var->kind == OBJ_VARIABLE) {
    // Nếu là biến: kiểu có thể là mảng
    varType = var->varAttrs->type;
    // Phân tích các index nếu là mảng
    varType = compileIndexes(varType);
  } else if (var->kind == OBJ_PARAMETER) {
    // Nếu là tham số: lấy kiểu của tham số
    varType = var->paramAttrs->type;
  } else if (var->kind == OBJ_FUNCTION) {
    // Nếu là hàm: lấy kiểu trả về của hàm (chỉ hàm hiện tại)
    varType = var->funcAttrs->returnType;
  }

  return varType;
}

/**
 * compileAssignSt()
 * ------------------
 * Hàm phân tích câu lệnh gán
 * Cấu trúc: <lvalue> := <expression>
 * 
 * Xử lý:
 * 1. Phân tích left value (biến, phần tử mảng, ...)
 * 2. Phân tích right value (biểu thức)
 * 3. Kiểm tra kiểu của left value và right value phải bằng nhau
 */
void compileAssignSt(void) {
  Type* lvalueType = compileLValue();
  eat(SB_ASSIGN);  // :=
  Type* exprType = compileExpression();
  // Kiểm tra kiểu nhất quán: left value type == right value type
  checkTypeEquality(lvalueType, exprType);
}

/**
 * compileCallSt()
 * ----------------
 * Hàm phân tích câu lệnh gọi procedure
 * Cấu trúc: CALL <id> [(<arguments>)]
 * 
 * Xử lý:
 * 1. Tiêu thụ CALL
 * 2. Tiêu thụ tên procedure
 * 3. Kiểm tra procedure có được khai báo không
 * 4. Phân tích các arguments
 */
void compileCallSt(void) {
  Object* proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  // Kiểm tra tên procedure có được khai báo không
  proc = checkDeclaredProcedure(currentToken->string);

  // Phân tích các arguments (tuần tự với danh sách tham số)
  compileArguments(proc->procAttrs->paramList);
}

/**
 * compileGroupSt()
 * -----------------
 * Hàm phân tích khối lệnh (compound statement)
 * Cấu trúc: BEGIN <statements> END
 * 
 * Là cách để nhóm nhiều câu lệnh thành một
 */
void compileGroupSt(void) {
  eat(KW_BEGIN);
  // Phân tích danh sách các câu lệnh
  compileStatements();
  eat(KW_END);
}

/**
 * compileIfSt()
 * ---------------
 * Hàm phân tích câu lệnh if
 * Cấu trúc: IF <condition> THEN <statement> [ELSE <statement>]
 * 
 * Xử lý:
 * 1. Phân tích điều kiện (phải là basic type)
 * 2. Phân tích câu lệnh thực hiện nếu đúng
 * 3. Nếu có ELSE: phân tích câu lệnh thực hiện nếu sai
 */
void compileIfSt(void) {
  eat(KW_IF);
  // Phân tích điều kiện (phải là INTEGER hoặc CHAR)
  compileCondition();
  eat(KW_THEN);
  // Phân tích câu lệnh nếu điều kiện đúng
  compileStatement();
  // Nếu có ELSE: phân tích câu lệnh nếu điều kiện sai
  if (lookAhead->tokenType == KW_ELSE) 
    compileElseSt();
}

/**
 * compileElseSt()
 * ----------------
 * Hàm phân tích phần ELSE của câu lệnh IF
 * Cấu trúc: ELSE <statement>
 */
void compileElseSt(void) {
  eat(KW_ELSE);
  // Phân tích câu lệnh thực hiện nếu điều kiện sai
  compileStatement();
}

/**
 * compileWhileSt()
 * -----------------
 * Hàm phân tích câu lệnh while
 * Cấu trúc: WHILE <condition> DO <statement>
 * 
 * Xử lý:
 * 1. Phân tích điều kiện (phải là basic type)
 * 2. Phân tích câu lệnh thực hiện lặp
 */
void compileWhileSt(void) {
  eat(KW_WHILE);
  // Phân tích điều kiện (phải là INTEGER hoặc CHAR)
  compileCondition();
  eat(KW_DO);
  // Phân tích câu lệnh thực hiện lặp
  compileStatement();
}

/**
 * compileForSt()
 * ----------------
 * Hàm phân tích câu lệnh for
 * Cấu trúc: FOR <id> := <expr> TO <expr> DO <statement>
 * 
 * Xử lý:
 * 1. Phân tích biến vòng lặp (phải là INTEGER)
 * 2. Phân tích giá trị bắt đầu (phải là INTEGER)
 * 3. Phân tích giá trị kết thúc (phải là INTEGER)
 * 4. Phân tích câu lệnh thực hiện lặp
 */
void compileForSt(void) {
  Object* var;
  Type* varType;
  Type* exprType;
  
  eat(KW_FOR);
  eat(TK_IDENT);

  // Kiểm tra biến vòng lặp phải là biến INTEGER
  var = checkDeclaredVariable(currentToken->string);
  varType = var->varAttrs->type;
  checkIntType(varType);  // Phải là INTEGER

  eat(SB_ASSIGN);  // :=
  // Phân tích giá trị bắt đầu (phải là INTEGER)
  exprType = compileExpression();
  checkTypeEquality(varType, exprType);

  eat(KW_TO);
  // Phân tích giá trị kết thúc (phải là INTEGER)
  exprType = compileExpression();
  checkIntType(exprType);

  eat(KW_DO);
  // Phân tích câu lệnh thực hiện lặp
  compileStatement();
}

/**
 * compileArgument()
 * ------------------
 * Hàm phân tích một argument của function call hoặc procedure call
 * Cấu trúc: <expression>
 * 
 * Xử lý:
 * 1. Phân tích biểu thức
 * 2. Kiểm tra kiểu của biểu thức phải khớp với kiểu tham số tương ứng
 * 
 * @param param: Object tham số để kiểm tra kiểu
 */
void compileArgument(Object* param) {
  // Phân tích biểu thức (argument)
  Type* argType = compileExpression();
  // Kiểm tra kiểu argument phải khớp với kiểu tham số
  checkTypeEquality(param->paramAttrs->type, argType);
}

/**
 * compileArguments()
 * -------------------
 * Hàm phân tích danh sách các arguments của function/procedure call
 * Cấu trúc: ( <argument> [, <argument>]* ) hoặc rỗng
 * 
 * Xử lý:
 * 1. Nếu có dấu mở ngoặc: phân tích danh sách arguments
 * 2. Kiểm tra số arguments và kiểu của mỗi argument
 * 
 * @param paramList: Danh sách tham số của hàm/procedure
 */
void compileArguments(ObjectNode* paramList) {
  ObjectNode* paramNode = paramList;  // Node tham số hiện tại
  
  switch (lookAhead->tokenType) {
  case SB_LPAR:  // (
    eat(SB_LPAR);
    // Phân tích argument đầu tiên (nếu có tham số)
    if (paramNode != NULL) {
      compileArgument(paramNode->object);
      paramNode = paramNode->next;
    }

    // Phân tích các arguments tiếp theo
    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      if (paramNode != NULL) {
        compileArgument(paramNode->object);
        paramNode = paramNode->next;
      }
    }
    
    eat(SB_RPAR);  // )
    break;
    // Kiểm tra FOLLOW set - các token có thể đến sau arguments rỗng
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    // Không có arguments
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

/**
 * compileCondition()
 * -------------------
 * Hàm phân tích điều kiện
 * Cấu trúc: <expression> <comparator> <expression>
 * 
 * Các comparator: =, <>, <=, <, >=, >
 * 
 * Xử lý:
 * 1. Phân tích biểu thức trái (phải là basic type)
 * 2. Phân tích toán tử so sánh
 * 3. Phân tích biểu thức phải (phải là basic type)
 * 4. Kiểm tra kiểu của cả hai biểu thức phải bằng nhau
 */
void compileCondition(void) {
  // Phân tích biểu thức bên trái
  Type* type1 = compileExpression();
  checkBasicType(type1);  // Phải là INTEGER hoặc CHAR

  // Phân tích toán tử so sánh
  switch (lookAhead->tokenType) {
  case SB_EQ:  // =
    eat(SB_EQ);
    break;
  case SB_NEQ:  // <>
    eat(SB_NEQ);
    break;
  case SB_LE:  // <=
    eat(SB_LE);
    break;
  case SB_LT:  // <
    eat(SB_LT);
    break;
  case SB_GE:  // >=
    eat(SB_GE);
    break;
  case SB_GT:  // >
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  // Phân tích biểu thức bên phải
  Type* type2 = compileExpression();
  checkBasicType(type2);  // Phải là INTEGER hoặc CHAR
  // Kiểm tra kiểu bằng nhau
  checkTypeEquality(type1, type2);
}

/**
 * compileExpression()
 * --------------------
 * Hàm phân tích biểu thức
 * Cấu trúc: [+ | -] <expr2>
 * 
 * Xử lý dấu unary + và -
 */
Type* compileExpression(void) {
  Type* type;
  
  switch (lookAhead->tokenType) {
  case SB_PLUS:  // Dấu cộng unary
    eat(SB_PLUS);
    type = compileExpression2();
    checkIntType(type);  // Phải là INTEGER
    break;
  case SB_MINUS:  // Dấu trừ unary
    eat(SB_MINUS);
    type = compileExpression2();
    checkIntType(type);  // Phải là INTEGER
    break;
  default:
    type = compileExpression2();
  }
  return type;
}

/**
 * compileExpression2()
 * ---------------------
 * Hàm phân tích biểu thức cộng/trừ
 * Cấu trúc: <term> [+ <term> | - <term>]*
 * 
 * Xử lý:
 * 1. Phân tích term đầu tiên
 * 2. Nếu có + hoặc -: phân tích các term tiếp theo
 */
Type* compileExpression2(void) {
  Type* type1;
  Type* type2;

  type1 = compileTerm();
  // Kiểm tra xem có phép cộng/trừ không
  type2 = compileExpression3();
  if (type2 == NULL) return type1;
  else {
    // Kiểu của cả hai phần phải bằng nhau
    checkTypeEquality(type1,type2);
    return type1;
  }
}

/**
 * compileExpression3()
 * ---------------------
 * Hàm phân tích các phép cộng/trừ
 * Cấu trúc: [+ <term> | - <term>]*
 * 
 * Xử lý lặp các phép cộng/trừ
 */
Type* compileExpression3(void) {
  Type* type1;
  Type* type2;

  switch (lookAhead->tokenType) {
  case SB_PLUS:  // Phép cộng
    eat(SB_PLUS);
    type1 = compileTerm();
    checkIntType(type1);  // Phải là INTEGER
    // Lặp tiếp các phép cộng/trừ
    type2 = compileExpression3();
    if (type2 != NULL)
      checkIntType(type2);
    return type1;
    break;
  case SB_MINUS:  // Phép trừ
    eat(SB_MINUS);
    type1 = compileTerm();
    checkIntType(type1);  // Phải là INTEGER
    // Lặp tiếp các phép cộng/trừ
    type2 = compileExpression3();
    if (type2 != NULL)
      checkIntType(type2);
    return type1;
    break;
    // Kiểm tra FOLLOW set
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    // Không có phép cộng/trừ nữa
    return NULL;
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    return NULL;
  }
}

/**
 * compileTerm()
 * ---------------
 * Hàm phân tích term (nhân/chia)
 * Cấu trúc: <factor> [* <factor> | / <factor>]*
 * 
 * Xử lý:
 * 1. Phân tích factor đầu tiên
 * 2. Nếu có * hoặc /: phân tích các factor tiếp theo
 */
Type* compileTerm(void) {
  Type* type;

  // Phân tích factor đầu tiên
  type = compileFactor();
  // Kiểm tra xem có phép nhân/chia không
  compileTerm2();

  return type;
}

/**
 * compileTerm2()
 * ----------------
 * Hàm phân tích các phép nhân/chia
 * Cấu trúc: [* <factor> | / <factor>]*
 * 
 * Xử lý lặp các phép nhân/chia
 */
void compileTerm2(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case SB_TIMES:  // Phép nhân
    eat(SB_TIMES);
    type = compileFactor();
    checkIntType(type);  // Phải là INTEGER
    // Lặp tiếp các phép nhân/chia
    compileTerm2();
    break;
  case SB_SLASH:  // Phép chia
    eat(SB_SLASH);
    type = compileFactor();
    checkIntType(type);  // Phải là INTEGER
    // Lặp tiếp các phép nhân/chia
    compileTerm2();
    break;
    // Kiểm tra FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    // Không có phép nhân/chia nữa
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
}

/**
 * compileFactor()
 * ----------------
 * Hàm phân tích nhân tử (factor) - mức cao nhất của ưu tiên toán tử
 * Có thể là: số, ký tự, biến, phần tử mảng, tham số, gọi hàm, hoặc biểu thức trong ngoặc
 * 
 * Trả về: Type* của factor
 */
Type* compileFactor(void) {
  Object* obj;
  Type* type = NULL;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:  // Số nguyên
    eat(TK_NUMBER);
    type = makeIntType();
    break;
  case TK_CHAR:  // Ký tự
    eat(TK_CHAR);
    type = makeCharType();
    break;
  case TK_IDENT:  // Identifier (biến, hằng, hàm, tham số, ...)
    eat(TK_IDENT);
    // Kiểm tra identifier có được khai báo không
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind) {
    case OBJ_CONSTANT:  // Hằng số
      if (obj->constAttrs->value->type == TP_INT)
        type = makeIntType();
      else
        type = makeCharType();
      break;
    case OBJ_VARIABLE:  // Biến (có thể là mảng)
      type = obj->varAttrs->type;
      // Phân tích các index nếu là mảng
      type = compileIndexes(type);
      break;
    case OBJ_PARAMETER:  // Tham số
      type = obj->paramAttrs->type;
      break;
    case OBJ_FUNCTION:  // Gọi hàm
      type = obj->funcAttrs->returnType;
      // Phân tích các arguments của hàm
      compileArguments(obj->funcAttrs->paramList);
      break;
    default: 
      error(ERR_INVALID_FACTOR,currentToken->lineNo, currentToken->colNo);
      break;
    }
    break;
  case SB_LPAR:  // Biểu thức trong ngoặc
    eat(SB_LPAR);  // (
    type = compileExpression();  // Phân tích biểu thức
    eat(SB_RPAR);  // )
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
  
  return type;
}

/**
 * compileIndexes()
 * ------------------
 * Hàm phân tích danh sách index cho truy cập mảng
 * Cấu trúc: [ <expression> ] [ <expression> ] ...
 * 
 * Xử lý:
 * 1. Lặp trong khi còn [
 * 2. Kiểm tra kiểu hiện tại phải là mảng
 * 3. Phân tích expression (phải là INTEGER)
 * 4. Cập nhật kiểu thành kiểu phần tử
 * 
 * @param arrayType: Kiểu mảng ban đầu
 * @return: Kiểu phần tử sau khi tính tất cả các index
 */
Type* compileIndexes(Type* arrayType) {
  Type* currentType = arrayType;
  Type* indexType;
  
  while (lookAhead->tokenType == SB_LSEL) {  // [
    // Kiểm tra kiểu hiện tại phải là mảng
    checkArrayType(currentType);
    eat(SB_LSEL);  // [
    // Phân tích index (phải là INTEGER)
    indexType = compileExpression();
    checkIntType(indexType);
    eat(SB_RSEL);  // ]
    // Chuyển đến kiểu phần tử
    currentType = currentType->elementType;
  }
  
  return currentType;
}

/**
 * compile(char *fileName)
 * -------------------------
 * Hàm chính để bắt đầu quá trình phân tích cú pháp
 * 
 * Quy trình:
 * 1. Mở file input
 * 2. Khởi tạo tokens (currentToken và lookAhead)
 * 3. Khởi tạo bảng ký hiệu
 * 4. Gọi compileProgram() để phân tích chương trình
 * 5. In ra danh sách các objects đã khai báo
 * 6. Giải phóng bộ nhớ
 * 7. Đóng file
 * 
 * @param fileName: Đường dẫn file chương trình KPL
 * @return: IO_SUCCESS nếu thành công, IO_ERROR nếu không thể mở file
 */
int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  // Khởi tạo tokens
  currentToken = NULL;
  lookAhead = getValidToken();  // Token đầu tiên

  // Khởi tạo bảng ký hiệu toàn cục
  initSymTab();

  // Bắt đầu phân tích chương trình
  compileProgram();

  // In ra danh sách các objects khai báo (nếu kết quả không lỗi)
  printObject(symtab->program,0);

  // Giải phóng bảng ký hiệu
  cleanSymTab();

  // Giải phóng tokens
  free(currentToken);
  free(lookAhead);
  // Đóng file input
  closeInputStream();
  return IO_SUCCESS;

}
