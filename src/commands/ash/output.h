

#include "stdarg.h"

typedef struct {
	char *nextChar;
	int numLeft;
	char *buf;
	int bufSize;
	short fd;
	short flags;
} Output;

extern Output output;
extern Output errOut;
extern Output memOut;
extern Output *out1;
extern Output *out2;

void outStr(char *, Output *);
void out1Str(char *);
void out2Str(char *);
void outFormat(Output *, char *, ...);
void out1Format(char *, ...);
void formatStr(char *, int, char *, ...);
void doFormat(Output *, char *, va_list);
void emptyOutBuf(Output *);
void flushAll();
void flushOut(Output *);
void freeStdout();
int xwrite(int, char *, int);

#define outChar(c, file)	(--(file)->numLeft < 0 ? (emptyOutBuf(file), *(file)->nextChar++ = (c)) : \
							 (*(file)->nextChar++ = (c)))
#define out1Char(c)		outChar(c, out1);
#define out2Char(c)		outChar(c, out2);
