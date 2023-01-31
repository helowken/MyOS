#include "unistd.h"
#include "errno.h"

int readlink(const char *path, char *buf, size_t bufsiz) {
	errno = EINVAL;	/* "The named file is not a symbolic link" */
	return -1;
}

