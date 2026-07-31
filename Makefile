# Makefile for the Borno programming language
CC      = gcc
CFLAGS  = -Wall -Wextra -O2

borno: parser.tab.c lex.yy.c interpreter.c borno.h
	$(CC) $(CFLAGS) -o borno parser.tab.c lex.yy.c interpreter.c -lm

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

clean:
	rm -f borno parser.tab.c parser.tab.h lex.yy.c

.PHONY: clean
