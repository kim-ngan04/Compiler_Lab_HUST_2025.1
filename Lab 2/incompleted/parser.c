/* * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdlib.h>
#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "error.h"

// Biến toàn cục lưu trạng thái hiện tại của trình phân tích
Token *currentToken; // Token đang được xử lý (thực ra code này ít dùng biến này, chủ yếu dùng lookAhead)
Token *lookAhead;    // Token sắp tới (đang nhìn vào để quyết định đi hướng nào)

/* * Hàm scan:
 * Di chuyển "cửa sổ" đọc sang token tiếp theo.
 * Giải phóng bộ nhớ của token cũ và lấy token mới từ scanner.
 */
void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken(); // Lấy token hợp lệ tiếp theo từ scanner
  free(tmp);
}

/* * Hàm eat:
 * Kiểm tra xem token hiện tại (lookAhead) có đúng là loại (tokenType) mong đợi không.
 * - Nếu đúng: In token ra (để debug) và gọi scan() để đi tiếp.
 * - Nếu sai: Báo lỗi cú pháp (Missing token).
 * Đây là hàm cơ bản nhất để "tiêu thụ" các từ khóa, dấu câu.
 */
void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    printToken(lookAhead);
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

/* * compileProgram:
 * Hàm bắt đầu (Start symbol) của ngữ pháp.
 * Quy tắc: PROGRAM <tên> ; <Block> .
 */
void compileProgram(void) {
  assert("Parsing a Program ....");
  eat(KW_PROGRAM);   // Mong đợi từ khóa PROGRAM
  eat(TK_IDENT);     // Mong đợi tên chương trình (Identifier)
  eat(SB_SEMICOLON); // Mong đợi dấu chấm phẩy ;
  compileBlock();    // Phân tích khối lệnh chính
  eat(SB_PERIOD);    // Mong đợi dấu chấm . kết thúc chương trình
  assert("Program parsed!");
}

/* * Chuỗi các hàm compileBlock (Block, Block2, Block3, Block4, Block5):
 * Mục đích: Xử lý các phần khai báo theo thứ tự bắt buộc của Pascal:
 * CONST -> TYPE -> VAR -> Subroutines (Hàm/Thủ tục) -> BEGIN...END
 * * Cách hoạt động: Nếu có khai báo phần đó thì xử lý rồi gọi hàm tiếp theo,
 * nếu không có thì nhảy thẳng sang hàm tiếp theo.
 */

// Xử lý phần khai báo hằng (CONST)
void compileBlock(void) {
  assert("Parsing a Block ....");
  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);
    compileConstDecl();   // Khai báo 1 hằng
    compileConstDecls();  // Khai báo các hằng tiếp theo (nếu có)
    compileBlock2();      // Chuyển sang phần Type
  } 
  else compileBlock2();   // Không có Const thì chuyển ngay sang Type
  assert("Block parsed!");
}

// Xử lý phần khai báo kiểu (TYPE)
void compileBlock2(void) {
  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);
    compileTypeDecl();
    compileTypeDecls();
    compileBlock3();
  } 
  else compileBlock3();
}

// Xử lý phần khai báo biến (VAR)
void compileBlock3(void) {
  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    compileVarDecl();
    compileVarDecls();
    compileBlock4();
  } 
  else compileBlock4();
}

// Xử lý phần khai báo chương trình con (Function/Procedure)
void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

// Xử lý thân chương trình (BEGIN ... END)
void compileBlock5(void) {
  eat(KW_BEGIN);
  compileStatements(); // Các câu lệnh bên trong
  eat(KW_END);
}

/* * Nhóm hàm xử lý khai báo Hằng, Kiểu, Biến:
 * Sử dụng đệ quy đuôi hoặc vòng lặp để xử lý danh sách các khai báo.
 */

// Lặp lại việc khai báo hằng miễn là còn gặp định danh (Identifier)
void compileConstDecls(void) {
  while (lookAhead->tokenType == TK_IDENT) compileConstDecl();
}

// Quy tắc: Tên = Giá trị ;
void compileConstDecl(void) {
  eat(TK_IDENT);
  eat(SB_EQ);
  compileConstant();
  eat(SB_SEMICOLON);
}

// Lặp lại khai báo kiểu
void compileTypeDecls(void) {
  while (lookAhead->tokenType == TK_IDENT) compileTypeDecl();
}

// Quy tắc: Tên = Kiểu ;
void compileTypeDecl(void) {
  eat(TK_IDENT);
  eat(SB_EQ);
  compileType();
  eat(SB_SEMICOLON);
}

// Lặp lại khai báo biến
void compileVarDecls(void) {
  while (lookAhead->tokenType == TK_IDENT) compileVarDecl();
}

// Quy tắc: Tên : Kiểu ;
void compileVarDecl(void) {
  eat(TK_IDENT);
  eat(SB_COLON);
  compileType();
  eat(SB_SEMICOLON);
}

/* * compileSubDecls:
 * Phân tích các hàm (Function) hoặc thủ tục (Procedure).
 * Đây là đệ quy: Sau khi phân tích xong 1 hàm/thủ tục, lại gọi lại chính nó 
 * để kiểm tra xem còn hàm/thủ tục nào phía sau không.
 */
void compileSubDecls(void) {
  assert("Parsing subtoutines ....");
  if (lookAhead->tokenType == KW_FUNCTION) {
    compileFuncDecl();
    compileSubDecls();
  } else if (lookAhead->tokenType == KW_PROCEDURE) {
      compileProcDecl();
      compileSubDecls();
  }
  assert("Subtoutines parsed ....");
}

// Quy tắc hàm: FUNCTION Tên(Tham số) : Kiểu trả về ; Block ;
void compileFuncDecl(void) {
  assert("Parsing a function ....");
  eat(KW_FUNCTION);
  eat(TK_IDENT);
  compileParams();
  eat(SB_COLON);
  compileBasicType();
  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  assert("Function parsed ....");
}

// Quy tắc thủ tục: PROCEDURE Tên(Tham số) ; Block ;
void compileProcDecl(void) {
  assert("Parsing a procedure ....");
  eat(KW_PROCEDURE);
  eat(TK_IDENT);
  compileParams();
  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  assert("Procedure parsed ....");
}

/* * Nhóm hàm xử lý Giá trị hằng và Kiểu dữ liệu
 */

// Xử lý hằng số không dấu (số, tên hằng, ký tự)
void compileUnsignedConstant(void) {
    switch (lookAhead->tokenType) {
      case TK_NUMBER: eat(TK_NUMBER); break;
      case TK_IDENT:  eat(TK_IDENT); break;
      case TK_CHAR:   eat(TK_CHAR); break;
      default:        error(ERR_INVALIDCONSTANT, lookAhead->lineNo, lookAhead->colNo); break;
    }
}

// Xử lý hằng số có thể có dấu (+, -)
void compileConstant(void) {
    TokenType type = lookAhead->tokenType;
    switch (type) {
        case TK_NUMBER: compileConstant2(); break;
        case SB_PLUS:
        case SB_MINUS:
            eat(type); // Ăn dấu + hoặc -
            compileConstant2();
            break;
        case TK_CHAR: eat(TK_CHAR); break;
        default:      compileConstant2(); break;
    }
}

// Hàm phụ trợ cho Constant (xử lý phần số hoặc định danh sau dấu)
void compileConstant2(void) {
    switch (lookAhead->tokenType) {
        case TK_NUMBER: eat(TK_NUMBER); break;
        case TK_IDENT:  eat(TK_IDENT); break;
        default:        error(ERR_INVALIDCONSTANT, lookAhead->lineNo, lookAhead->colNo); break;
      }
}

// Phân tích các loại kiểu dữ liệu (Integer, Char, Array, hoặc Type định nghĩa trước)
void compileType(void) {
    switch (lookAhead->tokenType) {
        case KW_INTEGER: eat(KW_INTEGER); break;
        case KW_CHAR:    eat(KW_CHAR); break;
        case KW_ARRAY:   // Khai báo mảng: ARRAY [Số] OF Kiểu
            eat(KW_ARRAY);
            eat(SB_LSEL);
            eat(TK_NUMBER);
            eat(SB_RSEL);
            eat(KW_OF);
            compileType();
            break;
        case TK_IDENT:   eat(TK_IDENT); break;
        default:         error(ERR_INVALIDTYPE, lookAhead->lineNo, lookAhead->colNo); break;
    }
}

// Chỉ chấp nhận kiểu cơ bản (Integer, Char) - dùng cho kiểu trả về của hàm
void compileBasicType(void) {
    switch (lookAhead->tokenType) {
      case KW_INTEGER: eat(KW_INTEGER); break;
      case KW_CHAR:    eat(KW_CHAR); break;
      default:         error(ERR_INVALIDBASICTYPE, lookAhead->lineNo, lookAhead->colNo); break;
    }
}

/* * Nhóm hàm xử lý Tham số (Parameters)
 * Ví dụ: (var x: integer; y: char)
 */
void compileParams(void) {
    if (lookAhead->tokenType == SB_LPAR) { // Nếu có dấu mở ngoặc (
      eat(SB_LPAR);
      compileParam();   // Xử lý tham số đầu tiên
      compileParams2(); // Xử lý các tham số tiếp theo
      eat(SB_RPAR);     // Đóng ngoặc )
    }
}

void compileParams2(void) {
    while (lookAhead->tokenType == SB_SEMICOLON) { // Ngăn cách bởi dấu chấm phẩy ;
      eat(SB_SEMICOLON);
      compileParam();
    }
}

// Một khai báo tham số: [VAR] Tên : Kiểu
void compileParam(void) {
    switch (lookAhead->tokenType) {
        case KW_VAR: // Tham biến
          eat(KW_VAR);
          eat(TK_IDENT);
          break;
        case TK_IDENT: // Tham trị
          eat(TK_IDENT);
          break;
        default:
          error(ERR_INVALIDPARAM, lookAhead->lineNo, lookAhead->colNo);
          break;
      }
    eat(SB_COLON);
    compileBasicType();
}

/* * Nhóm hàm xử lý Câu lệnh (Statements)
 */

void compileStatements(void) {
    compileStatement();
    compileStatements2();
}

// Xử lý danh sách câu lệnh ngăn cách bởi dấu chấm phẩy
void compileStatements2(void) {
    if (lookAhead->tokenType == SB_SEMICOLON) {
        eat(SB_SEMICOLON);
        compileStatement();
        compileStatements2();
    } else if (lookAhead->tokenType != KW_END) { 
        // Nếu không phải dấu ; và cũng không phải END -> Lỗi thiếu ;
        eat(SB_SEMICOLON); 
    }
}

// Điều phối (Dispatcher) để gọi hàm xử lý phù hợp dựa trên từ khóa bắt đầu câu lệnh
void compileStatement(void) {
    switch (lookAhead->tokenType) {
        case TK_IDENT:   compileAssignSt(); break; // Gán: x := ...
        case KW_CALL:    compileCallSt(); break;   // Gọi thủ tục: CALL ...
        case KW_BEGIN:   compileGroupSt(); break;  // Khối lệnh con: BEGIN ... END
        case KW_IF:      compileIfSt(); break;     // IF ...
        case KW_WHILE:   compileWhileSt(); break;  // WHILE ...
        case KW_FOR:     compileForSt(); break;    // FOR ...
        case SB_SEMICOLON: // Câu lệnh rỗng
        case KW_END:       // Gặp END thì dừng (để hàm cha xử lý)
        case KW_ELSE:      // Gặp ELSE thì dừng (để compileIf xử lý)
              break;
        default:
              error(ERR_INVALIDSTATEMENT, lookAhead->lineNo, lookAhead->colNo);
              break;
    }  
}

// Xử lý biến (có thể là biến đơn hoặc phần tử mảng A[i])
void compileVariable(void){
    eat(TK_IDENT);
    if (lookAhead->tokenType == SB_LSEL) compileIndexes();
};

// Lệnh gán: Biến := Biểu thức
void compileAssignSt(void) {
    assert("Parsing an assign statement ....");
    compileVariable();
    eat(SB_ASSIGN); // :=
    compileExpression();
    assert("Assign statement parsed ....");
}

// Lệnh gọi thủ tục: CALL Tên(Tham số)
void compileCallSt(void) {
    assert("Parsing a call statement ....");
    eat(KW_CALL);
    eat(TK_IDENT);
    compileArguments(); // Các đối số truyền vào
    assert("Call statement parsed ....");
}

// Khối lệnh ghép
void compileGroupSt(void) {
    assert("Parsing a group statement ....");
    eat(KW_BEGIN);
    compileStatements(); 
    eat(KW_END);
    assert("Group statement parsed ....");
}

// Lệnh IF: IF Điều_kiện THEN Lệnh [ELSE Lệnh]
void compileIfSt(void) {
    assert("Parsing an if statement ....");
    eat(KW_IF);
    compileCondition();
    eat(KW_THEN);
    compileStatement();
    compileElseSt(); // Xử lý phần ELSE (nếu có)
    assert("If statement parsed ....");
}

void compileElseSt(void) {
    if (lookAhead->tokenType == KW_ELSE) {
        eat(KW_ELSE);
        compileStatement();
    }
}

// Lệnh WHILE: WHILE Điều_kiện DO Lệnh
void compileWhileSt(void) {
    assert("Parsing a while statement ....");
    eat(KW_WHILE);
    compileCondition();
    eat(KW_DO);
    compileStatement();
    assert("While statement parsed ....");
}

// Lệnh FOR: FOR i := 1 TO n DO Lệnh
void compileForSt(void) {
    assert("Parsing a for statement ....");
    eat(KW_FOR);
    eat(TK_IDENT);
    eat(SB_ASSIGN);
    compileExpression(); // Giá trị khởi đầu
    eat(KW_TO);
    compileExpression(); // Giá trị kết thúc
    eat(KW_DO);
    compileStatement();
    assert("For statement parsed ....");
}

/* * Nhóm hàm xử lý Biểu thức (Expressions)
 * Tuân theo thứ tự ưu tiên toán tử (Precedence):
 * 1. Factor (Nhân tử): Số, Biến, Ngoặc (Cao nhất)
 * 2. Term (Số hạng): Nhân, Chia (*, /)
 * 3. Expression (Biểu thức): Cộng, Trừ (+, -)
 * 4. Condition (Điều kiện): So sánh (=, <, >, ...) (Thấp nhất)
 */

// Xử lý đối số truyền vào hàm/thủ tục
void compileArguments(void) {
    switch (lookAhead->tokenType) {
      case SB_LPAR:
        eat(SB_LPAR);
        compileExpression();
        compileArguments2();
        eat(SB_RPAR);
        break;
      // Tập Follow set: Những token có thể xuất hiện nếu không có đối số
      case SB_TIMES: case SB_SLASH:
      case SB_PLUS: case SB_MINUS:
      case KW_TO: case KW_DO: case KW_END: case KW_ELSE: case KW_THEN:
      case SB_EQ: case SB_NEQ: case SB_LE: case SB_LT: case SB_GE: case SB_GT:
      case SB_RPAR: case SB_RSEL: case SB_COMMA: case SB_SEMICOLON:
        break;
      default:
        error(ERR_INVALIDARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
        break;
    }
}

// Các đối số tiếp theo ngăn cách bởi dấu phẩy
void compileArguments2(void) {
    switch (lookAhead->tokenType) {
      case SB_COMMA:
        eat(SB_COMMA);
        compileExpression();
        compileArguments2();
        break;
      case SB_RPAR: // Gặp dấu đóng ngoặc thì dừng
        break;
      default:
        error(ERR_INVALIDARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
        break;
    }
}

// Điều kiện: Biểu thức1 Toán_tử_so_sánh Biểu thức2
void compileCondition(void) {
    compileExpression();
    compileCondition2();
}

void compileCondition2(void) {
    switch (lookAhead->tokenType) {
      case SB_EQ: eat(SB_EQ); break;
      case SB_NEQ: eat(SB_NEQ); break;
      case SB_GT: eat(SB_GT); break;
      case SB_GE: eat(SB_GE); break;
      case SB_LT: eat(SB_LT); break;
      case SB_LE: eat(SB_LE); break;
      default:
        error(ERR_INVALIDCOMPARATOR, lookAhead->lineNo, lookAhead->colNo);
        break;
    }
    compileExpression();
}

// Biểu thức: Term + Term - Term ...
void compileExpression(void) {
    assert("Parsing an expression");
    switch (lookAhead->tokenType)
    {
    case SB_PLUS:  // Trường hợp số dương +5
        eat(SB_PLUS);
        compileExpression2();
        break;
    case SB_MINUS: // Trường hợp số âm -5
        eat(SB_MINUS);
        compileExpression2();
        break;
    default:
        compileExpression2();
        break;
    }
    assert("Expression parsed");
}

void compileExpression2(void) {
    compileTerm();       // Ưu tiên xử lý nhân chia trước
    compileExpression3(); // Sau đó mới xử lý cộng trừ
}

// Xử lý chuỗi cộng trừ liên tiếp (loại bỏ đệ quy trái)
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
        // Follow set: Các token báo hiệu kết thúc biểu thức
        case KW_TO: case KW_DO: case SB_RPAR: case SB_COMMA: case SB_EQ:
        case SB_NEQ: case SB_LE: case SB_LT: case SB_GE: case SB_GT:
        case SB_RSEL: case SB_SEMICOLON: case KW_END: case KW_ELSE: case KW_THEN:
            break;
        default:
            error(ERR_INVALIDEXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    }
}

// Term: Factor * Factor / Factor ...
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
        // Follow set
        case SB_PLUS: case SB_MINUS:
        case KW_TO: case KW_DO: case KW_END: case KW_ELSE: case KW_THEN:
        case SB_EQ: case SB_NEQ: case SB_LE: case SB_LT: case SB_GE: case SB_GT:
        case SB_RPAR: case SB_RSEL: case SB_COMMA: case SB_SEMICOLON:
        break;
        default:
        error(ERR_INVALIDTERM, lookAhead->lineNo, lookAhead->colNo);
        break;
        }
}
  
// Factor: Thành phần nhỏ nhất (Số, Char, Ident, hoặc Biểu thức trong ngoặc)
void compileFactor(void) {
    switch (lookAhead->tokenType) {
      case TK_NUMBER: eat(TK_NUMBER); break;
      case TK_CHAR: eat(TK_CHAR); break;
  
      case TK_IDENT:
        eat(TK_IDENT);
        // Kiểm tra xem Ident này là biến mảng (có [...]) hay hàm (có (...))
        switch (lookAhead->tokenType) {
          case SB_LSEL: compileIndexes(); break;   // A[i]
          case SB_LPAR: compileArguments(); break; // Func(a,b)
          default: break;
        }
        break;
  
      case SB_LPAR: // ( Biểu thức )
        eat(SB_LPAR);
        compileExpression();
        eat(SB_RPAR);
        break;
      default:
        error(ERR_INVALIDFACTOR, lookAhead->lineNo, lookAhead->colNo);
        break;
    }
}

// Xử lý chỉ số mảng: [EXP]
void compileIndexes(void) {
    while (lookAhead->tokenType == SB_LSEL) {
        eat(SB_LSEL);
        compileExpression();
        eat(SB_RSEL);
    }
}

/* * Hàm chính compile:
 * Mở file, khởi tạo scanner, bắt đầu parse từ compileProgram.
 */
int compile(char *fileName) {
    if (openInputStream(fileName) == IO_ERROR) return IO_ERROR;

    currentToken = NULL;
    lookAhead = getValidToken(); // Lấy token đầu tiên
    compileProgram();            // Bắt đầu phân tích

    // printf("Compile (Parsing) successfully!\n");

    free(currentToken);
    free(lookAhead);
    closeInputStream();
    return IO_SUCCESS;
}