#include "stdio.h"

#define ioTestFlag(p, x)	((p)->_flags & (x))

#include "stdarg.h"

int _doPrint(const char *format, va_list ap, FILE *stream);
int _doScan(FILE *stream, const char *format, va_list ap);
char *_i_compute(unsigned long val, int base, char *s, int numDigits);
char *_f_print(va_list *ap, int flags, char *s, char c, int precision);
void __cleanup();

FILE *popen(const char *command, const char *type);
FILE *fdopen(int fd, const char *mode);

#ifndef NOFLOAT
char *_ecvt(long double value, int nDigit, int *decpt, int *sign);
char *_fcvt(long double value, int nDigit, int *decpt, int *sign);
#endif

#define FL_LJUST	0x0001		/* Left-justify field */
#define FL_SIGN		0x0002		/* Sign in signed conversions */


