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

#define isAlphaNum(c)	((__ctype + 1)[c] & (_U | _L | _N))
#define isAlpha(c)		((__ctype + 1)[c] & (_U | _L))
#define	isControl(c)	((__ctype + 1)[c] & _C)
#define isGraphic(c)	((__ctype + 1)[c] & (_U | _L | _N | _P))
#define isPunct(c)		((__ctype + 1)[c] & _P)
#define isSpace(c)		((__ctype + 1)[c] & _S)
#define isHexDigit(c)	((__ctype + 1)[c] & (_N | _X))

#define isDigit(c)		((unsigned) ((c) - '0') < 10)
#define isLower(c)		((unsigned) ((c) - 'a') < 26)
#define isUpper(c)		((unsigned) ((c) - 'A') < 26)
#define isPrint(c)		((unsigned) ((c) - ' ') < 95)
#define isAscii(c)		((unsigned) (c) < 128)

#endif
