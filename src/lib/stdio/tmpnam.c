#if defined(_POSIX_SOURCE)
#include <sys/types.h>
#endif

#include <stdio.h>
#include <string.h>
#include "loc_incl.h"

pid_t getpid(void);

char *tmpnam(char *s) {
	static char nameBuffer[L_tmpnam] = "/tmp/tmp.";
	static unsigned long count = 0;
	static char *name = NULL;

	if (! name) {
		name = nameBuffer + strlen(nameBuffer);
		name = _i_compute((unsigned long) getpid(), 10, name, 5);
		*name++ = '.';
		*name = '\0';
	}
	if (++count > TMP_MAX)
	  count = 1;	/* Wrap-around */
	*_i_compute(count, 10, name, 3) = '\0';
	if (s)
	  return strcpy(s, nameBuffer);
	return nameBuffer;
}
