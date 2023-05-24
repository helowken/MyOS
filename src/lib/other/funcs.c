#include <lib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

ssize_t stdErr(const char *s) {
	return write(STDERR_FILENO, s, strlen(s));
}

ssize_t tell(const char *s) {
	return write(STDOUT_FILENO, s, strlen(s));
}

void reportStdErr(const char *prog, const char *s) {
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
	reportStdErr(prog, s);
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

char *getProg(char **argv) {
	char *prog;

	if ((prog = strrchr(argv[0], '/')) == NULL)
	  prog = argv[0];
	else
	  ++prog;
	return prog;
}
