

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
