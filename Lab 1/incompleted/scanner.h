#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"  
#include "charcode.h"
#include "reader.h"

// khai báo các hàm được gọi trong unit test
void skipBlank(void);
void skipComment(void);
Token* readIdentKeyword(void);
Token* readNumber(void);
Token* readConstChar(void);
Token* getToken(void);
Token* getValidToken(void);
int scan(char *filename);
void printToken(Token *token);

#endif
