#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <setjmp.h>
#include <utime.h>
#include <dirent.h>

#define MAX_ERROR	4

char *prog;
int errCnt = 0, subTest;
int m = 0xFFFF;
int cd = 1;

void setup(int argc, char **argv) {
	char name[10];
	char cmd[100];
	char *s;
	size_t n;

	prog = strrchr(argv[0], '/');
	if (prog)
	  ++prog;
	else
	  prog = argv[0];

	n = strspn(prog, "test");
	s = &prog[n];

	sync();

	if (argc == 2)
	  m = atoi(argv[1]);

	printf("Test  %s ", s);
	fflush(stdout);		/* Have to flush for child's benefit */

	if (cd) {
		sprintf(name, "DIR_%s%s", strlen(s) == 1 ? "0" : "", s);
		sprintf(cmd, "rm -rf %s; mkdir %s", name, name);
		system(cmd);
		chdir(name);
	}
}

void quit() {
	if (cd) {
		chdir("..");
		system("rm -rf DIR*");
	}

	if (errCnt == 0) {
		printf("ok\n");
		exit(EXIT_SUCCESS);
	} else {
		printf("%d errors\n", errCnt);
		exit(4);
	}
}

void fail(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	++errCnt;
	quit();
}

void e(int n) {
	int errNum = errno;

	printf("Subtest %d,  error %d  errno=%d  ", subTest, n, errno);
	errno = errNum;
	perror("");
	if (errCnt++ > MAX_ERROR) {
		printf("Too many errors;  test aborted\n");
		chdir("..");
		system("rm -rf DIR*");
		exit(EXIT_FAILURE);
	}
}

void fatal(char *s) {
	int e = errno;
	printf("%s: %s (%d)\n", s, strerror(e), e);
	exit(EXIT_FAILURE);
}

void forkFailed() {
	fatal("Fork failed");
}


