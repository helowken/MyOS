#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "loc_incl.h"

#define setPointer(flags)	(flags |= FL_LONG)
#define NUM_LEN		512
#define NR_CHARS	256

static char XTable[NR_CHARS];
static char inputBuf[NUM_LEN];

static char *ordinaryCollect(register int c, register FILE *stream, char type,
					unsigned int width, int *basePtr) {
/* Collect a number of characters which constitute an ordinal number.
 * When the type is 'i', the base can be 8, 10, or 16, depending on the
 * first 1 or 2 characters. This means that the base must be adjusted
 * acoording to the format of the number. At the end of the function, base
 * is then set to 0, so strtol() will get the right argument.
 */
	register char *bufPtr = inputBuf;
	register int base;

	switch (type) {
		case 'i':	/* i means octal, decimal or hexadecimal */
		case 'p':
		case 'x':
		case 'X': base = 16; break;
		case 'd':
		case 'u': base = 10; break;
		case 'o': base = 8; break;
		case 'b': base = 2; break;
	}

	if (c == '-' || c == '+') {
		*bufPtr++ = c;
		if (--width)
		  c = getc(stream);
	}

	if (width && c == '0' && base == 16) {
		*bufPtr++ = c;
		if (--width)
		  c = getc(stream);
		if (c != 'x' && c != 'X') {
			if (type == 'i')
			  base = 8;
		} else if (width) {
			*bufPtr++ = c;
			if (--width) 
			  c = getc(stream);
		}
	} else if (type == 'i') {
		base = 10;
	}

	while (width) {
		if ((base == 10 && isdigit(c)) ||
				(base == 16 && isxdigit(c)) ||
				(base == 8 && isdigit(c) && c < '8') ||
				(base == 2 && isdigit(c) && c < '2')) {
			*bufPtr++ = c;
			if (--width)
			  c = getc(stream);
		} else {
			break;
		}
	}

	if (width && c != EOF)
	  ungetc(c, stream);
	if (type == 'i')
	  base = 0;		/* Determined by inputBuf */
	*basePtr = base;
	*bufPtr = '\0';
	return bufPtr - 1;
}

static char *floatCollect(register int c, register FILE *stream, 
					register unsigned int width) {
/* Reads a string that has the format of a floating-point number. The 
 * function returns as soon as a format-error is encountered, leaving
 * the offending character in the input. This means that 1.el leaves 
 * the 'l' in the input queue. Since all detection of format errors is
 * done here, _doScan() doesn't call strtod() when it's not necessary,
 * although the use of the width field can cause incomplete numbers to
 * be passed to strtod(). (e.g. 1.3e+)
 */
	return NULL;//TODO 
}

int _doScan(FILE *stream, const char *format, va_list ap) {
	int done = 0;		/* Number of items done */
	int numChars = 0;	/* Number of characters read */
	int conv = 0;		/* # of conversions */
	int base;			/* Conversion base */
	unsigned long val;	/* An integer value */
	register char *str;	/* Tmporary pointer */
	char *tmpString;	/* Ditto */
	unsigned width = 0;		/* Width of field */
	int flags;			/* Some flags */
	int reverse;		/* Reverse the checking in [...] */
	int kind;
	register int ic = EOF;	/* The input character */
	long double ldVal;

	if (format == NULL || *format == 0)
	  return 0;

	while (1) {
		if (isspace(*format)) {
			while (isspace(*format)) {
				++format;	/* Skip whitespace */
			}
			ic = getc(stream);
			++numChars;
			while (isspace(ic)) {
				ic = getc(stream);
				++numChars;
			}
			if (ic != EOF)
			  ungetc(ic, stream);
			--numChars;
		}
		if (*format == 0)
		  break;	/* End of format */

		if (*format != '%') {
			ic = getc(stream);
			++numChars;
			if (ic != *format++)
			  break;	/* Error */
			continue;
		}
		++format;
		if (*format == '%') {	/* %% */
			ic = getc(stream);
			++numChars;
			if (ic == '%') {
				++format;
				continue;
			}
			else 
			  break;
		}
		flags = 0;
		if (*format == '*') {
			++format;
			flags |= FL_NO_ASSIGN;
		}
		if (isdigit(*format)) {
			flags |= FL_WIDTH_SPEC;
			for (width = 0; isdigit(*format); ) {
				width = width * 10 + (*format++ - '0');	
			}
		}

		switch (*format) {
			case 'h': flags |= FL_SHORT; ++format; break;
			case 'l': flags |= FL_LONG; ++format; break;
			case 'L': flags |= FL_LONG_DOUBLE; ++format; break;
		}
		kind = *format;
		if (kind != 'c' && kind != '[' && kind != 'n') {
			do {
				ic = getc(stream);
				++numChars;
			} while (isspace(ic));
			if (ic == EOF)
			  break;				/* Outer while */
		} else if (kind != 'n') {	/* %c or %[ */
			ic = getc(stream);	
			if (ic == EOF)			/* Outer while */
			  break;
			++numChars;
		}
		switch (kind) {
			default:
				/* Not recognized, like %q */
				return conv || (ic != EOF) ? done : EOF;
			case 'n':
				if (! (flags & FL_NO_ASSIGN)) {	/* Silly though */
					if (flags & FL_SHORT)
					  *va_arg(ap, short *) = (short) numChars;
					else if (flags & FL_LONG)
					  *va_arg(ap, long *) = (long) numChars;
					else
					  *va_arg(ap, int *) = (int) numChars;
				}
				break;
			case 'p':	/* Pointer */
				setPointer(flags);
				/* Fall through */
			case 'b':	/* Binary */
			case 'd':	/* Decimal */
			case 'i':	/* General integer */
			case 'o':	/* Octal */
			case 'u':	/* Unsigned */
			case 'x':	/* Hexadecimal */
			case 'X':	/* ditto */
				if (! (flags & FL_WIDTH_SPEC) || width > NUM_LEN)
				  width = NUM_LEN;
				if (width == 0)
				  return done;

				str = ordinaryCollect(ic, stream, kind, width, &base);
				if (str < inputBuf ||
						(str == inputBuf && (*str == '-' || *str == '+')))
				  return done;

				/* Although the length of the number is: str - inputBuf + 1
				 * we don't add the 1 since we counted it already
				 */
				numChars += str - inputBuf;

				if (! (flags & FL_NO_ASSIGN)) {
					if (kind == 'd' || kind == 'i')
					  val = strtol(inputBuf, &tmpString, base);
					else
					  val = strtoul(inputBuf, &tmpString, base);
					if (flags & FL_LONG)
					  *va_arg(ap, unsigned long *) = (unsigned long) val;
					else if (flags & FL_SHORT)
					  *va_arg(ap, unsigned short *) = (unsigned short) val;
					else
					  *va_arg(ap, unsigned *) = (unsigned) val;
				}
				break;
			case 'c':
				if (! (flags & FL_WIDTH_SPEC))
				  width = 1;
				if (! (flags & FL_NO_ASSIGN))
				  str = va_arg(ap, char *);
				if (! width)
				  return done;

				while (width && ic != EOF) {
					if (! (flags & FL_NO_ASSIGN))
					  *str++ = (char) ic;
					if (--width) {
						ic = getc(stream);
						++numChars;
					}
				}

				if (width) {
					if (ic != EOF) 
					  ungetc(ic, stream);
					--numChars;
				}
				break;
			case 's':
				if (! (flags & FL_WIDTH_SPEC))
				  width = 0xffff;
				if (! (flags & FL_NO_ASSIGN))
				  str = va_arg(ap, char *);
				if (! width)
				  return done;

				while (width && ic != EOF && ! isspace(ic)) {
					if (! (flags & FL_NO_ASSIGN))
					  *str++ = (char) ic;
					if (--width) {
						ic = getc(stream);
						++numChars;
					}
				}
				/* Terminate the string */
				if (! (flags & FL_NO_ASSIGN)) 
				  *str = '\0';
				if (width) {
					if (ic != EOF)
					  ungetc(ic, stream);
					--numChars;
				}
				break;
			case '[':
				if (! (flags & FL_WIDTH_SPEC))
				  width = 0xffff;
				if (! width)
				  return done;

				if (*++format == '^') {
					reverse = 1;
					++format;
				} else {
					reverse = 0;
				}

				for (str = XTable; str < &XTable[NR_CHARS]; ++str) {
					*str = 0;
				}
				if (*format == ']')
				  XTable[(int) *format++] = 1;

				while (*format && *format != ']') {
					XTable[(int) *format++] = 1;
					if (*format == '-') {
						++format;
						if (*format && 
								*format != ']' &&
								*format >= *(format - 2)) {
							int c;
							for (c = *(format - 2) + 1; c <= *format; ++c) {
								XTable[c] = 1;
							}
							++format;
						} else {
							XTable['-'] = 1;
						}
					}
				}
				if (! *format)
				  return done;

				if (! (XTable[ic] ^ reverse)) {
					/* MAT 8/9/96 no match must return character */
					ungetc(ic, stream);
					return done;
				}

				if (! (flags & FL_NO_ASSIGN))
				  str = va_arg(ap, char *);

				do {
					if (! (flags & FL_NO_ASSIGN))
					  *str++ = (char) ic;
					if (--width) {
						ic = getc(stream);
						++numChars;
					}
				} while (width && ic != EOF && (XTable[ic] ^ reverse));

				if (width) {
					if (ic != EOF)
					  ungetc(ic, stream);
					--numChars;
				}
				if (! (flags & FL_NO_ASSIGN)) 
				  *str = '\0';
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				if (! (flags & FL_WIDTH_SPEC) || width > NUM_LEN)
				  width = NUM_LEN;

				if (! width)
				  return done;
				str = floatCollect(ic, stream, width);

				if (str < inputBuf || 
						(str == inputBuf && (*str == '-' || *str == '+')))
				  return done;

				/* Although the length of the number is: str - inputBuf + 1
				 * we don't add the 1 since we counted it already
				 */
				numChars += str - inputBuf;

				if (! (flags & FL_NO_ASSIGN)) {
					ldVal = strtod(inputBuf, &tmpString);
					if (flags & FL_LONG_DOUBLE)
					  *va_arg(ap, long double *) = (long double) ldVal;
					else if (flags & FL_LONG)
					  *va_arg(ap, double *) = (double) ldVal;
					else
					  *va_arg(ap, float *) = (float) ldVal;
				}
				break;
		}
		++conv;
		if (! (flags & FL_NO_ASSIGN) && kind != 'n')
		  ++done;
		++format;
	}
	return conv || (ic != EOF ? done : EOF);
}
