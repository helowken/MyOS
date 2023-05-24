#define nil	0
#include <lib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

DIR *opendir(const char *name) {
/* Open a directory for reading. */
	int fd, fdes;
	struct stat st;
	DIR *dp;
	int err;
	
	/* Only read directories. */
	if (stat(name, &st) < 0)
	  return nil;
	if (! S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		return nil;
	}

	if ((fd = open(name, O_RDONLY | O_NONBLOCK)) < 0)
	  return nil;

	/* Check the type again, mark close-on-exec, get a buffer. */
	if (fstat(fd, &st) < 0 ||
			(errno = ENOTDIR, ! S_ISDIR(st.st_mode)) ||
			(fdes = fcntl(fd, F_GETFD)) < 0 ||
			fcntl(fd, F_SETFD, fdes | FD_CLOEXEC) < 0 ||
			(dp = (DIR *) malloc(sizeof(*dp))) == nil) {
		err = errno;
		close(fd);
		errno = err;
		return nil;
	}

	errno = 0;	/* Not to affect the caller's logic. */

	dp->_fd = fd;
	dp->_v7 = -1;
	dp->_count = 0;
	dp->_pos = 0;
	return dp;
}
