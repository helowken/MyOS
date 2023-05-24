#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE	1
#endif

#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <minix/minlib.h>

static char dir[PATH_MAX + 1];

int main(int argc, char **argv) {
	char *p;

	p = getcwd(dir, PATH_MAX);
	if (p == NULL) 
	  fatal(argv[0], "cannot search some directory on the path\n");
	tell(p);
	tell("\n");
	return 0;
}
