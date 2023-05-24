#include <stdio.h>

#define ioTestFlag(p, x)	((p)->_flags & (x))

#include <stdarg.h>

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

#define FL_LJUST		0x0001	/* Left-justify field */
#define FL_SIGN			0x0002	/* Sign in signed conversions */
#define FL_SPACE		0x0004	/* Space in signed conversions */
#define FL_ALT			0x0008	/* Alternate form */
#define FL_ZERO_FILL	0x0010	/* Fill with zero's */
#define FL_SHORT		0x0020	/* Optional h */
#define FL_LONG			0x0040	/* Optional l */
#define FL_LONG_DOUBLE	0x0080	/* Optional L */
#define FL_WIDTH_SPEC	0x0100	/* Field with is specified */
#define FL_PREC_SPEC	0x0200	/* Precision is specified */
#define FL_SIGNED_CONV	0x0400	/* May contain a sign */
#define FL_NO_ASSIGN	0x0800	/* Do not assign (in scanf) */
#define FL_NO_MORE		0x1000	/* All flags collected */

