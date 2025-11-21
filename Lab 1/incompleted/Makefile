CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm 

all: scanner

scanner: scanner.o reader.o charcode.o token.o error.o main.o
	${CC} scanner.o reader.o charcode.o token.o error.o main.o -o scanner.exe

unit_test_scanner: unit_test_scanner.c reader.o charcode.o token.o error.o
	${CC} unit_test_scanner.c reader.o charcode.o token.o error.o -lcmocka -o unit_test_scanner.exe

reader.o: reader.c
	${CC} ${CFLAGS} reader.c

scanner.o: scanner.c
	${CC} ${CFLAGS} scanner.c

charcode.o: charcode.c
	${CC} ${CFLAGS} charcode.c

token.o: token.c
	${CC} ${CFLAGS} token.c

error.o: error.c
	${CC} ${CFLAGS} error.c

clean:
	rm -f *.o *~ *.exe
