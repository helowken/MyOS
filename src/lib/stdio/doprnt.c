#include "ctype.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "loc_incl.h"

/* getNum() is used to get the width and precision fields of a format. */
static const char *getNum(register const char *fmt, int *ip, 
			va_list *app) {
	register int i, c;

	if (*fmt == '*') {
		*ip = va_arg(*app, int);
		++fmt;
	} else {
		i = 0;
		while ((c = *fmt - '0') >= 0 && c <= 9) {
			i = i * 10 + c;
			++fmt;
		}
		*ip = i;
	}
	return fmt;
}

#define setPointer(flags)	(flags |= FL_LONG)

/* Print an ordinal number. */
static char *ordinalPrint(va_list *ap, int flags, char *s, char c, 
			int precision, int isSigned) {
	long signedVal;
	unsigned long unsignedVal;
	char *oldStr = s;
	int base;

	switch (flags & (FL_SHORT | FL_LONG)) {
		case FL_SHORT:
			if (isSigned)
			  signedVal = (short) va_arg(*ap, int);
			else 
			  unsignedVal = (unsigned short) va_arg(*ap, unsigned);
			break;
		case FL_LONG:
			if (isSigned)
			  signedVal = va_arg(*ap, long);
			else 
			  unsignedVal = va_arg(*ap, unsigned long);
			break;
		default:
			if (isSigned)
			  signedVal = va_arg(*ap, int);
			else 
			  unsignedVal = va_arg(*ap, unsigned int);
			break;
	}

	if (isSigned) {
		if (signedVal < 0) {
			*s++ = '-';
			signedVal = -signedVal;
		} else if (flags & FL_SIGN) {
			*s++ = '+';
		} else if (flags & FL_SPACE) {
			*s++ = ' ';	
		}
		unsignedVal = signedVal;
	}

	if ((flags & FL_ALT) && c == 'o')
	  *s++ = '0';

	if (! unsignedVal) {
		if (! precision)
		  return s;
	} else if (((flags & FL_ALT) && (c == 'x' || c == 'X')) ||
					c == 'p') {
		*s++ = '0';
		*s++ = (c == 'X') ? 'X' : 'x';
	}

	switch (c) {
		case 'b':
			base = 2;
			break;
		case 'o':
			base = 8;
			break;
		case 'd':
		case 'i':
		case 'u':
			base = 10;
			break;
		case 'x':
		case 'X':
		case 'p':
			base = 16;
			break;
	}
	s = _i_compute(unsignedVal, base, s, precision);

	if (c == 'X') {
		while (oldStr != s) {
			*oldStr = toupper(*oldStr);
			++oldStr;
		}
	}
	return s;
}

int _doPrint(register const char *fmt, va_list ap, FILE *stream) {
	register char *s;
	register int j;
	int i, c, width, precision, zeroFill, flags, betweenFill;
	int numChars = 0;
	const char *oldFmt;
	char *s1, buf[1025];

	while ((c = *fmt++)) {
		if (c != '%') {
			if (putc(c, stream) == EOF)
			  return numChars ? -numChars : -1;
			++numChars;
			continue;
		}
		flags = 0;
		do {
			switch (*fmt) {
				case '-':
					flags |= FL_LJUST;
					break;
				case '+':
					flags |= FL_SIGN;
					break;
				case ' ':
					flags |= FL_SPACE;
					break;
				case '#':
					flags |= FL_ALT;
					break;
				case '0':
					flags |= FL_ZERO_FILL;
					break;
				default:
					flags |= FL_NO_MORE;
					continue;
			}
			++fmt;
		} while (! (flags & FL_NO_MORE));

		oldFmt = fmt;
		fmt = getNum(fmt, &width, &ap);
		if (fmt != oldFmt)
		  flags |= FL_WIDTH_SPEC;

		if (*fmt == '.') {
			++fmt;
			oldFmt = fmt;
			fmt = getNum(fmt, &precision, &ap);
			if (precision >= 0)
			  flags |= FL_PREC_SPEC;
		}

		if ((flags & FL_WIDTH_SPEC) && width < 0) {
			width = -width;
			flags |= FL_LJUST;
		}
		if (! (flags & FL_WIDTH_SPEC))
		  width = 0;

		if (flags & FL_SIGN)
		  flags &= ~FL_SPACE;

		if (flags & FL_LJUST)
		  flags &= ~FL_ZERO_FILL;

		s = s1 = buf;

		switch (*fmt) {
			case 'h':
				flags |= FL_SHORT;
				++fmt;
				break;
			case 'l':
				flags |= FL_LONG;
				++fmt;
				break;
			case 'L':
				flags |= FL_LONG_DOUBLE;
				++fmt;
				break;
		}

		switch ((c = *fmt++)) {
			default:
				if (putc(c, stream) == EOF)
				  return numChars ? -numChars : -1;
				++numChars;
				continue;
			case 'n':
				if (flags & FL_SHORT)
				  *va_arg(ap, short *) = (short) numChars;
				else if (flags & FL_LONG)
				  *va_arg(ap, long *) = (long) numChars;
				else
				  *va_arg(ap, int *) = (int) numChars;
				continue;
			case 's':
				s1 = va_arg(ap, char *);
				if (s1 == NULL)
				  s1 = "(null)";
				s = s1;
				while (precision || ! (flags & FL_PREC_SPEC)) {
					if (*s == '\0')
					  break;
					++s;
					--precision;
				}
				break;
			case 'p':
				setPointer(flags);
				/* Fall through */
			case 'b':
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				if (! (flags & FL_PREC_SPEC))
				  precision = 1;
				else if (c != 'p')
				  flags &= ~FL_ZERO_FILL;
				s = ordinalPrint(&ap, flags, s, c, precision, 0);
				break;
			case 'd':
			case 'i':
				flags |= FL_SIGNED_CONV;
				if (! (flags & FL_PREC_SPEC))
				  precision = 1;
				else 
				  flags &= ~FL_ZERO_FILL;
				s = ordinalPrint(&ap, flags, s, c, precision, 1);
				break;
			case 'c':
				*s++ = va_arg(ap, int);
				break;
			case 'G':
			case 'g':
				if ((flags & FL_PREC_SPEC) && precision == 0)
				  precision = 1;
			case 'f':
			case 'E':
			case 'e':
				if (! (flags & FL_PREC_SPEC))
				  precision = 6;

				if (precision >= sizeof(buf))
				  precision = sizeof(buf) - 1;

				flags |= FL_SIGNED_CONV;
				s = _f_print(&ap, flags, s, c, precision);
				break;
			case 'r':
				ap = va_arg(ap, va_list);
				fmt = va_arg(ap, char *);
				continue;
		}
		zeroFill = ' ';
		if (flags & FL_ZERO_FILL)
		  zeroFill = '0';
		j = s - s1;

		/* betweenFill is true under the following conditions:
		 * 1-  the fill character is '0'
		 * and
		 * 2a- the number is of the form 0x... or 0X...
		 * or
		 * 2b- the number contains a sign or space
		 */
		betweenFill = 0;
		if ((flags & FL_ZERO_FILL) &&
				(((c == 'x' || c == 'X') && (flags & FL_ALT)) ||
					c == 'p' ||
					((flags & FL_SIGNED_CONV) && (*s1 == '+' || *s1 == '-' || *s1 == ' ')))
		   ) {
			++betweenFill;
		}

		if ((i = width - j) > 0) {
			if (! (flags & FL_LJUST)) {		/* Right justify */
				numChars += i;
				if (betweenFill) {
					if (flags & FL_SIGNED_CONV) {
						--j;
						++numChars;
					}
					if (putc(*s1++, stream) == EOF) {
						return numChars ? -numChars : -1;
					} else {
						j -= 2;
						numChars += 2;
						if (putc(*s1++, stream) == EOF ||
								putc(*s1++, stream) == EOF)
						  return numChars ? -numChars : -1;
					}
				}
				do {
					if (putc(zeroFill, stream) == EOF)
					  return numChars ? -numChars : -1;
				} while (--i);
			}
		}

		numChars += j;
		while (--j >= 0) {
			if (putc(*s1++, stream) == EOF) 
			  return numChars ? -numChars : -1;
		}

		if (i > 0)
		  numChars += i;
		while (--i >= 0) {
			if (putc(*s1++, stream) == EOF) 
			  return numChars ? -numChars : -1;
		}
	}
	return numChars;
}
