#include <lib.h>
#include <fcntl.h>
#include <unistd.h>

int dup2(int fd, int fd2) {
/* The behavior of dup2 is defined by POSIX as almost, 
 * but not quite the same as fcntl.
 */
	if (fd2 < 0 || fd2 > OPEN_MAX) {
		errno = EBADF;
		return -1;
	}

	/* Check to see if fd is valid. */
	if (fcntl(fd, F_GETFL) < 0) {
		/* 'fd' is not valid. */
		return -1;
	} else {
		/* 'fd' is valid. */
		if (fd == fd2)
		  return fd2;
		close(fd2);
		return fcntl(fd, F_DUPFD, fd2);
	}
}
