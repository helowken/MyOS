

#include "shell.h"
#include "syntax.h"
#include "error.h"
#include "mystring.h"


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

