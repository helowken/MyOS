


#include "../shell.h"
#include "../mystring.h"
#ifdef SHELL
#include "../output.h"
/*
#define stdout	out1
#define stderr	out2
#define printf	out1Format
#define putc(c, file)	outChar(c, file)
#define putchar(c)	out1Char(c)
#define fprintf	outFormat
#define fputs	outStr
#define fflush	flushOut
*/
#define INIT_ARGS(argv)
#else
#undef NULL
#include "stdio.h"
#undef main
#define INIT_ARGS(argv)		if ((commandName = argv[0]) == NULL) {fputs("Argc is zero\n", stderr); exit(2);} else
#endif

	
pointer stackAlloc(int);
void error(char *, ...);

extern char *commandName;
