#define _MINIX_SOURCE

#define nil	0
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

extern char * const **_penviron;	/* The default environment. */

int execvp(const char *file, char * const argv[]) {
/* Execute the file with a path search on $PATH, just like the shell. The
 * search continues on the errors ENOENT (not there), and EACCES (file not
 * executable or leading directories protected.)
 * Unlike other execvp implementations there is no default path, and no shell
 * is started for scripts. One is supposed to define $PATH, and use #!/bin/sh.
 */
	struct stat sb;
	const char *path;	/* $PATH */
	char *full;			/* Full name to try. */
	char *f;
	size_t fullSize;
	int err = ENOENT;	/* Error return on failure. */

	if (strchr(file, '/') != nil || (path = getenv("PATH")) == nil)
	  path = "";

	/* Compute the maximum length the full name may have, and align. */
	fullSize = strlen(path) + 1 + strlen(file) + 1;
	fullSize = (fullSize + sizeof(char *) - 1) & ~(sizeof(char *) - 1);

	/* Claim space. */
	if ((full = (char *) sbrk(fullSize)) == (char *) -1) {
		errno = E2BIG;
		return -1;
	}

	/* For each directory in the path... */
	do {
		f = full;
		while (*path != 0 && *path != ':') {
			*f++ = *path++;
		}
		if (f > full)
		  *f++ = '/';
		strcpy(f, file);

		/* Stat first, small speed-up, better for ptrace. */
		if (stat(full, &sb) == -1)
		  continue;

		execve(full, argv, *_penviron);

		/* Prefer more interesting errno values then "not there". */
		if (errno != ENOENT)
		  err = errno;

		/* Continue only on some errors. */
		if (err != ENOENT && err != EACCES)
		  break;
	} while (*path++ != 0);

	sbrk(-fullSize);
	errno = err;
	return -1;
}
