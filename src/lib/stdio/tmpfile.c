#if defined(_POSIX_SOURCE)
#include "sys/types.h"
#endif

#include "stdio.h"
#include "string.h"
#include "loc_incl.h"

pid_t getpid(void);

FILE *tmpfile() {
	static char nameBuffer[L_tmpnam] = "/tmp/tmp.";
	static char *name = NULL;
	FILE *file;

	if (! name) {
		name = nameBuffer + strlen(nameBuffer);
		name = _i_compute(getpid(), 10, name, 5);
		*name = '\0';
	}

	file = fopen(nameBuffer, "wb+");
	if (! file)
	  return (FILE *) NULL;
	remove(nameBuffer);
	return file;
}
