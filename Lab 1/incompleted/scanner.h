#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

void skipBlank(void);
void skipComment(void);
Token* readIdentKeyword(void);
Token* readNumber(void);
Token* readConstChar(void);
Token* getToken(void);
void printToken(Token *token);
int scan(char *fileName);

#endif
