#if defined(_POSIX_SOURCE)
#include "sys/types.h"
#endif

#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "loc_incl.h"

ssize_t write(int fd, const char *buf, size_t bytes);

void perror(const char *s) {
	char *p;
	int fd;

	p = strerror(errno);
	fd = fileno(stderr);
	fflush(stdout);
	fflush(stderr);
	if (s && *s) {
		write(fd, s, strlen(s));
		write(fd, ": ", 2);
	}
	write(fd, p, strlen(p));
	write(fd, "\n", 1);
}
