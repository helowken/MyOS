#ifndef _CTYPE_H
#define _CTYPE_H

extern char __ctype[];	/* Property array defined in chartab.c */

#define _U		0x01	/* This bit is for upper-case letters [A-Z]. */
#define _L		0x02	/* This bit is for lower-case letters [a-z]. */
#define _N		0x04	/* This bit is for numbers [0-9]. */
#define _S		0x08	/* This bit is for white space: \t, \n, \f etc. */
#define _P		0x10	/* This bit is for punctuation characters. */
#define _C		0x20	/* This bit is for control characters. */
#define _X		0x40	/* This bit is for hex digits [a-f] and [A-F]. */

/* Function Prototypes (have to go before the macros). */
int isalnum(int c);		/* Alphanumeric [a-z], [A-Z], [0-9] */
int isalpha(int c);		/* Alphabetic */
int iscntrl(int c);		/* Control characters */
int isdigit(int c);		/* Digit [0-9] */
int isgraph(int c);		/* Graphic character */
int islower(int c);		/* Lower-case letter [a-z] */
int isprint(int c);		/* Printable character */
int ispunct(int c);		/* Punctuation mark */
int isspace(int c);		/* White space sp, \f, \n, \r, \t, \v */
int isupper(int c);		/* Upper-case letter [A-Z] */
int isxdigit(int c);	/* Hex digit [0-9], [a-f], [A-F] */
int tolower(int c);		/* Convert to lower-case */
int toupper(int c);		/* Convert to upper-case */

#define isalnum(c)		((__ctype + 1)[(unsigned) c] & (_U | _L | _N))
#define isalpha(c)		((__ctype + 1)[(unsigned) c] & (_U | _L))
#define	iscntrl(c)		((__ctype + 1)[(unsigned) c] & _C)
#define isgraph(c)		((__ctype + 1)[(unsigned) c] & (_U | _L | _N | _P))
#define ispunct(c)		((__ctype + 1)[(unsigned) c] & _P)
#define isspace(c)		((__ctype + 1)[(unsigned) c] & _S)
#define isxdigit(c)		((__ctype + 1)[(unsigned) c] & (_N | _X))

#define isdigit(c)		((unsigned) ((c) - '0') < 10)
#define islower(c)		((unsigned) ((c) - 'a') < 26)
#define isupper(c)		((unsigned) ((c) - 'A') < 26)
#define isprint(c)		((unsigned) ((c) - ' ') < 95)
#define isAscii(c)		((unsigned) (c) < 128)

#endif
