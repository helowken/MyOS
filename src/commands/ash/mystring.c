

#include "stdlib.h"
#include "shell.h"
#include "syntax.h"
#include "error.h"
#include "mystring.h"


char nullStr[1];	/* Zero length string */


/* isPrefix -- see if pfx is a prefix os string.
 */
int isPrefix(register char const *pfx,
			register char const *string) {
	while (*pfx) {
		if (*pfx++ != *string++)
		  return 0;
	}
	return 1;
}

/* bcopy - copy bytes
 *
 * This routine are derived from code by Henry Spencer.
 */
void myBcopy(const pointer src, pointer dst, int len) {
	register char *d = dst;
	register char *s = src;

	while (--len >= 0) {
		*d++ = *s++;
	}
}

/* Convert a string of digits to an integer, printing an error 
 * message on failure.
 */
int number(const char *s) {
	if (! isNumber(s))
	  error2("Illegal number", (char *) s);
	return atoi(s);
}

/* Check for a valid number. This should be elsewhere.
 */
int isNumber(register const char *p) {
	do {
		if (! isDigit(*p))
		  return 0;
	} while (*++p != '\0');
	return 1;
}

/* prefix -- see if pfx is a prefix of string.
 */
int prefix(register const char *pfx, register const char *string) {
	while (*pfx) {
		if (*pfx++ != *string++)
		  return 0;
	}
	return 1;
}

