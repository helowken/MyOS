#define nil	0
#include <lib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int closedir(DIR *dp) {
/* Finish reading a directory. */
	int fd;

	if (dp == nil) {
		errno = EBADF;
		return -1;
	}

	fd = dp->_fd;
	free(dp);
	return close(fd);
}
