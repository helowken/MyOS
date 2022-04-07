#include "code.h"
#include "ctype.h"
#include "errno.h"
#include "limits.h"
#include "stdlib.h"

#define between(a, c, z)	((unsigned) ((c) - (a)) <= (unsigned) ((z) - (a)))

static unsigned long string2Long(register const char *nptr, char ** const endptr, 
			int base, int isSigned) {
	register unsigned int v;
	register unsigned long val = 0;
	register int c;
	int overflow = 0, sign = 1;
	const char *startnptr = nptr, *nrstart;

	if (endptr)
	  *endptr = (char *) nptr;

	/* Skip space */
	while (isSpace(*nptr)) {
		++nptr;
	}
	c = *nptr;

	/* Determine sign */
	if (c == '-' || c == '+') {
		if (c == '-')
		  sign = -1;
		++nptr;
	}
	nrstart = nptr;			/* start of the number */

	/* When base is 0, the syntax determines the actual base. */
	if (base == 0) {
		if (*nptr == '0') {
			++nptr;
			if (*nptr == 'x' || *nptr == 'X') {
			  base = 16;
			  ++nptr;
			} else
			  base = 8;
		} else
		  base = 10;
	} else if (base == 16 && *nptr == '0') {
		++nptr;
		if (*nptr == 'x' || *nptr == 'X')
		  ++nptr;
	}
	
	while (1) {
		c = *nptr;
		if (between('0', c, '9'))
		  v = c - '0';
		else if (between('a', c, 'z'))
		  v = c - 'a' + 0xa;
		else if (between('A', c, 'Z'))
		  v = c - 'A' + 0xA;
		else
		  break;	/* No more parsing */

		if (v >= base)
		  break;
		if (val > (ULONG_MAX - v) / base)
		  ++overflow;
		val = val * base + v;
		++nptr;
	}
	if (endptr) {
		if (nrstart == nptr)
		  *endptr = (char *) startnptr;
		else
		  *endptr = (char *) nptr;
	}
	
	if (!overflow && isSigned) {
		/* See the assembly code. 
		 * -(unsigned long) LONG_MIN equals (unsigned long) LONG_MIN
		 */
		if ((sign < 0 && val > -(unsigned long) LONG_MIN) ||
			(sign > 0 && val > LONG_MAX))
		  ++overflow;
	}

	if (overflow) {
		errno = ERANGE;
		if (isSigned)
		  return sign < 0 ? LONG_MIN : LONG_MAX;
		return ULONG_MAX;
	}
	return (long) sign * val;
}

long int strtol(const char *nptr, char **endptr, int base) {
	return (signed long) string2Long(nptr, endptr, base, 1);
}

unsigned long int strtoul(const char *nptr, char **endptr, int base) {
	return (unsigned long) string2Long(nptr, endptr, base, 0);
}


