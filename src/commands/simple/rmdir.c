#define nil	0
#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <minix/minlib.h>

void main(int argc, char **argv) {
	int i, ex = 0;
	char *prog;

	prog = getProg(argv);
	if (argc < 2) 
	  usage(prog, "directory ...");

	i = 1;
	do {
		if (rmdir(argv[i]) < 0) {
			reportStdErr(prog, argv[i]);
			ex = 1;
		}
	} while (++i < argc);

	exit(ex);
}

