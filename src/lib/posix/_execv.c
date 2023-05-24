#include <unistd.h>

extern char * const **_penviron;	/* The default environment. */

int execv(const char *path, char * const argv[]) {
	return execve(path, argv, *_penviron);	
}
