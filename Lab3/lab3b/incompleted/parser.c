/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 * 
 * FILE: parser.c
 * MỤC ĐÍCH:
 *   - Phân tích cú pháp (syntax analysis)
 *   - Xây dựng cây phân tích cú pháp từ các token
 *   - Quản lý bảng ký hiệu (symbol table)
 *   - Kiểm tra lỗi ngữ nghĩa (semantic checking)
 * 
 * CÁC HÀNG: Parser dùng LL(1) lookahead parsing
 *   - currentToken: token hiện tại đã xử lý
 *   - lookAhead: token tiếp theo cần xử lý (nhìn trước 1 token)
 * 
 * CẤU TRÚC CHƯƠNG TRÌNH:
 *   - compileProgram(): xử lý cấu trúc chương trình
 *   - compileBlock(): xử lý khối khai báo (const, type, var)
 *   - compileSubDecls(): xử lý khai báo hàm/thủ tục
 *   - compileStatements(): xử lý các lệnh
 */

#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "error.h"
#include "debug.h"

/* Biến global cho lookahead parsing */
Token *currentToken;  /* Token hiện tại đã xử lý */
Token *lookAhead;     /* Token tiếp theo (nhìn trước) */

/* Các kiểu cơ bản được import từ symtab.c */
extern Type* intType;
extern Type* charType;
extern SymTab* symtab;

/******* HÀNG HỖ TRỢ LL(1) *******/

/* 
 * HÀM scan: Di chuyển sang token tiếp theo
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Giải phóng currentToken
 *   2. currentToken = lookAhead
 *   3. lookAhead = getValidToken() (lấy token mới từ scanner)
 * 
 * MỤC ĐÍCH:
 *   - Duy trì 2 token: currentToken (vừa xử lý) và lookAhead (tiếp theo)
 *   - Cho phép parser nhìn trước 1 token để quyết định parse rule
 * 
 * VÍ DỤ:
 *   Ban đầu: currentToken = NULL, lookAhead = "PROGRAM"
 *   Sau scan(): currentToken = "PROGRAM", lookAhead = "X"
 *   Sau scan(): currentToken = "X", lookAhead = ";"
 */
void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();  /* Lấy token mới từ scanner */
  free(tmp);  /* Giải phóng token cũ */
}

/* 
 * HÀM eat: Tiêu thụ token (consume token)
 * 
 * THAM SỐ:
 *   - tokenType: loại token mong đợi
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Kiểm tra xem lookAhead có phải tokenType không
 *   2. Nếu có → gọi scan() để di chuyển sang token tiếp theo
 *   3. Nếu không → báo lỗi missingToken()
 * 
 * MỤC ĐÍCH:
 *   - Xác nhận token và di chuyển tiếp
 *   - Đảm bảo cú pháp đúng
 * 
 * VÍ DỤ:
 *   lookAhead = ";"
 *   eat(SB_SEMICOLON)  → scan() → nextToken
 *   
 *   lookAhead = ","
 *   eat(SB_SEMICOLON)  → báo lỗi missingToken()
 */
void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    scan();  /* Token đúng → di chuyển tiếp */
  } else 
    missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);  /* Token sai → báo lỗi */
}

/* 
 * HÀM compileProgram: Xử lý cấu trúc chương trình
 * 
 * CÚ PHÁP:
 *   PROGRAM <ident> ;
 *   <block>
 *   .
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Tiêu thụ "PROGRAM"
 *   2. Tiêu thụ identifier (tên chương trình)
 *   3. Tạo object chương trình
 *   4. Vào scope toàn cục của chương trình
 *   5. Tiêu thụ ";"
 *   6. Xử lý block (khai báo + lệnh)
 *   7. Tiêu thụ "." (kết thúc chương trình)
 *   8. Thoát scope
 * 
 * VÍ DỤ:
 *   PROGRAM X;
 *   CONST c1 = 10;
 *   BEGIN ... END.
 */
void compileProgram(void) {
  Object* program;

  eat(KW_PROGRAM);  /* Tiêu thụ "PROGRAM" */
  eat(TK_IDENT);    /* Tiêu thụ tên chương trình */

  /* Tạo object chương trình với tên từ currentToken */
  program = createProgramObject(currentToken->string);
  
  /* Vào scope toàn cục */
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);  /* Tiêu thụ ";" */

  compileBlock();     /* Xử lý block khai báo */
  
  eat(SB_PERIOD);     /* Tiêu thụ "." (kết thúc) */

  exitBlock();        /* Thoát scope */
}

/* 
 * HÀM compileBlock: Xử lý khối khai báo (const, type, var, hàm/thủ tục)
 * 
 * CÚ PHÁP:
 *   [CONST <const-decl> {; <const-decl>}]
 *   [TYPE <type-decl> {; <type-decl>}]
 *   [VAR <var-decl> {; <var-decl>}]
 *   [<subdecls>]
 *   BEGIN <statements> END
 * 
 * CÁCH HOẠT ĐỘNG:
 *   - Xử lý khai báo hằng (nếu có CONST)
 *   - Gọi compileBlock2() để xử lý phần còn lại
 *   - compileBlock2() xử lý TYPE
 *   - compileBlock3() xử lý VAR
 *   - compileBlock4() xử lý hàm/thủ tục
 *   - compileBlock5() xử lý phần BEGIN...END
 * 
 * VÍ DỤ:
 *   CONST
 *     c1 = 10;
 *     c2 = 'a';
 *   TYPE
 *     ...
 *   VAR
 *     ...
 */
void compileBlock(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);  /* Tiêu thụ "CONST" */

    /* Xử lý các khai báo hằng */
    do {
      eat(TK_IDENT);  /* Tiêu thụ tên hằng */
      
      checkFreshIdent(currentToken->string);  /* Kiểm tra tên chưa được khai báo */
      constObj = createConstantObject(currentToken->string);  /* Tạo object hằng */
      
      eat(SB_EQ);  /* Tiêu thụ "=" */
      constValue = compileConstant();  /* Xử lý giá trị hằng */
      
      constObj->constAttrs->value = constValue;  /* Gán giá trị */
      declareObject(constObj);  /* Khai báo hằng */
      
      eat(SB_SEMICOLON);  /* Tiêu thụ ";" */
    } while (lookAhead->tokenType == TK_IDENT);  /* Nếu còn identifier → tiếp tục

    compileBlock2();  /* Xử lý phần còn lại */
  } 
  else 
    compileBlock2();  /* Không có CONST → xử lý TYPE/VAR */
}

void compileBlock2(void) {
  /* 
   * Xử lý khai báo TYPE
   * Nếu có TYPE → xử lý, sau đó gọi compileBlock3
   * Nếu không → bỏ qua, gọi compileBlock3
   */
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);

    do {
      eat(TK_IDENT);
      
      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);
      
      eat(SB_EQ);
      actualType = compileType();
      
      typeObj->typeAttrs->actualType = actualType;
      declareObject(typeObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock3();
  } 
  else compileBlock3();
}

void compileBlock3(void) {
  /* 
   * Xử lý khai báo VAR
   * Nếu có VAR → xử lý, sau đó gọi compileBlock4
   * Nếu không → bỏ qua, gọi compileBlock4
   */
  Object* varObj;
  Type* varType;

  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);

    do {
      eat(TK_IDENT);
      
      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);

      eat(SB_COLON);
      varType = compileType();
      
      varObj->varAttrs->type = varType;
      declareObject(varObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock4();
  } 
  else compileBlock4();
}

void compileBlock4(void) {
  /* 
   * Xử lý khai báo hàm/thủ tục
   * compileSubDecls(): xử lý các FUNCTION/PROCEDURE
   */
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void) {
  /* 
   * Xử lý phần chương trình chính (BEGIN...END)
   */
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

/* 
 * HÀM compileSubDecls: Xử lý khai báo hàm/thủ tục
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Duyệt xem có FUNCTION hoặc PROCEDURE không
 *   2. Nếu có: gọi compileFuncDecl() hoặc compileProcDecl()
 *   3. Lặp lại đến khi không có FUNCTION/PROCEDURE
 */
void compileSubDecls(void) {
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else compileProcDecl();
  }
}

/* 
 * HÀM compileFuncDecl: Xử lý khai báo hàm
 * 
 * CÚ PHÁP:
 *   FUNCTION <ident> ( [<params>] ) : <basic-type> ;
 *   <block> ;
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Tiêu thụ "FUNCTION" và tên hàm
 *   2. Tạo object hàm
 *   3. Khai báo hàm vào scope
 *   4. Vào scope của hàm
 *   5. Xử lý tham số (nếu có)
 *   6. Tiêu thụ ":" và xử lý kiểu trả về
 *   7. Tiêu thụ ";"
 *   8. Xử lý block (khai báo + lệnh)
 *   9. Tiêu thụ ";" kết thúc hàm
 *   10. Thoát scope
 * 
 * VÍ DỤ:
 *   FUNCTION ADD(a: INTEGER; b: INTEGER) : INTEGER;
 *   VAR x: INTEGER;
 *   BEGIN x := a + b END ;
 */
void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);  /* Vào scope hàm */
  
  compileParams();  /* Xử lý tham số */

  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();  /* Thoát scope hàm */
}

void compileProcDecl(void) {
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);  /* Vào scope thủ tục */

  compileParams();  /* Xử lý tham số */

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();  /* Thoát scope thủ tục */
}

/* 
 * HÀM compileUnsignedConstant: Xử lý hằng số không dấu
 * 
 * CÚ PHÁP:
 *   <unsigned-constant> ::= <number> | <ident> | <char>
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xem loại token tiếp theo
 *   2. Nếu TK_NUMBER: tạo hằng số integer
 *      - eat(TK_NUMBER)
 *      - makeIntConstant(currentToken->value)
 *   3. Nếu TK_IDENT: lấy giá trị từ constant đã khai báo
 *      - eat(TK_IDENT)
 *      - checkDeclaredConstant() kiểm tra identifier
 *      - duplicateConstantValue() sao chép giá trị
 *   4. Nếu TK_CHAR: tạo hằng số char
 *      - eat(TK_CHAR)
 *      - makeCharConstant(currentToken->string[0])
 *   5. Mặc định: báo lỗi ERR_INVALID_CONSTANT
 * 
 * VÍ DỤ:
 *   CONST c1 = 10;          // 10 là number
 *   CONST c2 = c1;          // c1 là ident
 *   CONST c3 = 'A';         // 'A' là char
 */
ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

/* 
 * HÀM compileConstant: Xử lý hằng số (có thể có dấu)
 * 
 * CÚ PHÁP:
 *   <constant> ::= [+ | -] <unsigned-constant> | <char>
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xem token tiếp theo
 *   2. Nếu "+": gọi compileConstant2() (unsigned constant)
 *   3. Nếu "-": gọi compileConstant2(), sau đó đảo dấu giá trị
 *   4. Nếu "'": tạo hằng số char
 *   5. Mặc định: gọi compileConstant2()
 * 
 * VÍ DỤ:
 *   CONST c1 = +10;          // dương
 *   CONST c2 = -5;           // âm
 *   CONST c3 = 'X';          // ký tự
 *   CONST c4 = 100;          // không dấu
 */
ConstantValue* compileConstant(void) {
  ConstantValue* constValue;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    constValue->intValue = - constValue->intValue;  /* Đảo dấu */
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

/* 
 * HÀM compileConstant2: Xử lý hằng số nguyên không dấu
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xem loại token
 *   2. TK_NUMBER: tạo hằng nguyên
 *   3. TK_IDENT: kiểm tra phải là constant nguyên, sao chép giá trị
 *   4. Mặc định: báo lỗi
 * 
 * GHI CHÚ:
 *   - Khác với compileUnsignedConstant: không cho TK_CHAR
 *   - Dùng khi xử lý hằng với dấu
 */
ConstantValue* compileConstant2(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
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

/* 
 * HÀM compileType: Xử lý khai báo kiểu
 * 
 * CÚ PHÁP:
 *   <type> ::= <basic-type> | ARRAY [ <number> ] OF <type>
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xem loại token
 *   2. KW_INTEGER: tạo kiểu INT
 *   3. KW_CHAR: tạo kiểu CHAR
 *   4. KW_ARRAY: tạo kiểu ARRAY
 *      - eat(KW_ARRAY)
 *      - eat(SB_LSEL) → tiêu thụ "["
 *      - eat(TK_NUMBER) → lấy kích thước
 *      - eat(SB_RSEL) → tiêu thụ "]"
 *      - eat(KW_OF)
 *      - elementType = compileType() → xử lý kiểu phần tử (đệ quy)
 *      - type = makeArrayType(arraySize, elementType)
 *   5. TK_IDENT: lấy kiểu từ type đã khai báo
 *   6. Mặc định: báo lỗi
 * 
 * VÍ DỤ:
 *   TYPE Matrix = ARRAY [10] OF ARRAY [20] OF INTEGER;
 *   VAR arr: ARRAY [5] OF CHAR;
 */
Type* compileType(void) {
  Type* type;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER);
    type =  makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    if (obj != NULL)
      type = duplicateType(obj->typeAttrs->actualType);
    else
      type = makeIntType();
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    type = makeIntType();
    break;
  }
  return type;
}

/* 
 * HÀM compileBasicType: Xử lý kiểu cơ bản
 * 
 * CÚ PHÁP:
 *   <basic-type> ::= INTEGER | CHAR
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xem loại token
 *   2. KW_INTEGER: tạo kiểu INT
 *   3. KW_CHAR: tạo kiểu CHAR
 *   4. Mặc định: báo lỗi
 * 
 * GHI CHÚ:
 *   - Khác với compileType: không cho ARRAY, chỉ kiểu cơ bản
 *   - Dùng cho kiểu trả về của hàm
 * 
 * VÍ DỤ:
 *   FUNCTION ADD(x: INTEGER; y: INTEGER) : INTEGER;
 *                                           ^^^^^^^ basicType
 */
Type* compileBasicType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER); 
    type = makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

/* 
 * HÀM compileParams: Xử lý danh sách tham số hàm/thủ tục
 * 
 * CÚ PHÁP:
 *   ( [<param> {; <param>}] )
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Nếu có "(":
 *      - eat(SB_LPAR) → tiêu thụ "("
 *      - compileParam() → xử lý tham số 1
 *      - Lặp: nếu có ";" → eat, rồi compileParam() tiếp
 *      - eat(SB_RPAR) → tiêu thụ ")"
 *   2. Nếu không có → không có tham số
 * 
 * GHI CHÚ:
 *   - Tham số được ngăn cách bởi ";"
 *   - Mỗi tham số được xử lý bởi compileParam()
 * 
 * VÍ DỤ:
 *   FUNCTION F(x: INTEGER; y: CHAR; z: INTEGER) : INTEGER;
 *               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *               Đây là danh sách tham số
 */
void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

/* 
 * HÀM compileParam: Xử lý một tham số
 * 
 * CÚ PHÁP:
 *   [VAR] <ident> : <basic-type>
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xem loại tham số:
 *      - Nếu KW_VAR: eat(KW_VAR), paramKind = PARAM_REFERENCE (tham số tham chiếu)
 *      - Nếu TK_IDENT: paramKind = PARAM_VALUE (tham số truyền giá trị)
 *   2. eat(TK_IDENT) → lấy tên tham số
 *   3. checkFreshIdent() → kiểm tra tên chưa được khai báo
 *   4. createParameterObject() → tạo object tham số
 *   5. eat(SB_COLON)
 *   6. compileBasicType() → xử lý kiểu tham số
 *   7. declareObject() → khai báo tham số
 * 
 * VÍ DỤ:
 *   x: INTEGER       // tham số truyền giá trị
 *   VAR y: CHAR      // tham số tham chiếu (cho phép thay đổi)
 */
void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind;

  switch (lookAhead->tokenType) {
  case TK_IDENT:
    paramKind = PARAM_VALUE;  /* Tham số truyền giá trị */
    break;
  case KW_VAR:
    eat(KW_VAR);
    paramKind = PARAM_REFERENCE;  /* Tham số tham chiếu (VAR) */
    break;
  default:
    error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
    break;
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind, symtab->currentScope->owner);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

/* 
 * HÀM compileStatements: Xử lý danh sách lệnh
 * 
 * CÚ PHÁP:
 *   <statement> {; <statement>}
 * 
 * CÁCH HOẠT ĐỘNG:
 *   1. Xử lý lệnh đầu tiên (compileStatement)
 *   2. Lặp: nếu có ";" → eat, rồi compileStatement() tiếp
 */
void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

/* 
 * HÀM compileStatement: Xử lý một lệnh
 * 
 * CÚ PHÁP:
 *   <statement> ::= <assign-st> | <call-st> | <group-st> | <if-st> | 
 *                   <while-st> | <for-st> | <empty-st>
 * 
 * CÁCH HOẠT ĐỘNG:
 *   - Xem token tiếp theo, gọi hàm xử lý lệnh tương ứng
 *   - TK_IDENT: compileAssignSt() (gán)
 *   - KW_CALL: compileCallSt() (gọi thủ tục)
 *   - KW_BEGIN: compileGroupSt() (nhóm lệnh)
 *   - KW_IF: compileIfSt() (lệnh if)
 *   - KW_WHILE: compileWhileSt() (vòng while)
 *   - KW_FOR: compileForSt() (vòng for)
 *   - Khác (;, END, ELSE): lệnh rỗng (empty statement)
 */
void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    /* Lệnh rỗng: không làm gì */
    break;
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileLValue(void) {
  eat(TK_IDENT);
  compileIndexes();
}

void compileAssignSt(void) {
  compileLValue();
  eat(SB_ASSIGN);
  compileExpression();
}

void compileCallSt(void) {
  eat(KW_CALL);
  eat(TK_IDENT);
  compileArguments();
}

void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void) {
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE) 
    compileElseSt();
}

void compileElseSt(void) {
  eat(KW_ELSE);
  compileStatement();
}

void compileWhileSt(void) {
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

void compileForSt(void) {
  eat(KW_FOR);
  eat(TK_IDENT);
  eat(SB_ASSIGN);
  compileExpression();
  eat(KW_TO);
  compileExpression();
  eat(KW_DO);
  compileStatement();
}

void compileArgument(void) {
  compileExpression();
}

void compileArguments(void) {
  switch (lookAhead->tokenType) {
  case SB_LPAR:
    eat(SB_LPAR);
    compileArgument();

    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      compileArgument();
    }

    eat(SB_RPAR);
    break;
    // Check FOLLOW set 
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
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void) {
  compileExpression();
  switch (lookAhead->tokenType) {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  compileExpression();
}

void compileExpression(void) {
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileExpression2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileExpression2();
    break;
  default:
    compileExpression2();
  }
}

void compileExpression2(void) {
  compileTerm();
  compileExpression3();
}


void compileExpression3(void) {
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileTerm();
    compileExpression3();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileTerm();
    compileExpression3();
    break;
    // check the FOLLOW set
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
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileTerm(void) {
  compileFactor();
  compileTerm2();
}

void compileTerm2(void) {
  switch (lookAhead->tokenType) {
  case SB_TIMES:
    eat(SB_TIMES);
    compileFactor();
    compileTerm2();
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    compileFactor();
    compileTerm2();
    break;
    // check the FOLLOW set
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
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileFactor(void) {
  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    switch (lookAhead->tokenType) {
    case SB_LPAR:
      compileArguments();
      break;
    case SB_LSEL:
      compileIndexes();
      break;
    default:
      break;
    }
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileIndexes(void) {
  while (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    compileExpression();
    eat(SB_RSEL);
  }
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  printObject(symtab->program,0);

  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;

}
