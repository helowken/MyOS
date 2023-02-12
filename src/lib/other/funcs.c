#include "lib.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"

ssize_t stdErr(const char *s) {
	return write(STDERR_FILENO, s, strlen(s));
}

ssize_t tell(const char *s) {
	return write(STDOUT_FILENO, s, strlen(s));
}

void report(const char *prog, const char *s) {
	int err = errno;
	stdErr(prog);
	stdErr(": ");
	if (s != NULL) {
		stdErr(s);
		stdErr(": ");
	}
	stdErr(strerror(err));
	stdErr("\n");
	errno = err;
}

void fatal(const char *prog, const char *s) {
	report(prog, s);
	exit(1);
}

void usage(const char *prog, const char *s) {
	stdErr("Usage: ");
	stdErr(prog);
	stdErr(" ");
	stdErr(s);
	stdErr("\n");
	exit(1);
}

