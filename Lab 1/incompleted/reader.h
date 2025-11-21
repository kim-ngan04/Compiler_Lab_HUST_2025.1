/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#ifndef __READER_H__
#define __READER_H__

#define IO_ERROR 0
#define IO_SUCCESS 1

extern int lineNo;
extern int colNo;
extern int currentChar;

int readChar(void);
int openInputStream(char *fileName);
void closeInputStream(void);
void resetReaderState(void);

#endif
