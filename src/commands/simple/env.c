#define nil	0
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv) {
	int i;
	int iFlag = 0;
	int aFlag = 0;
	extern char **environ;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1;

		if (opt[0] == '-' && opt[1] == 0)	/* -- */
		  break;	
		if (opt[0] == 0)	/* - */
		  iFlag = 1;

		while (*opt != 0) {
			switch (*opt++) {
				case 'i':
					iFlag = 1;	/* Clear environment. */
					break;
				case 'a':
					aFlag = 1;
					break;
				default:
					fprintf(stderr,
				"Usage: env [-ia] [name=value] ... [utility [argument ...]]\n");
					exit(EXIT_FAILURE);
			}
		}
	}

	/* Clear the environment if -i. */
	if (iFlag)
	  *environ = nil;

	/* Set the new environment strings. */
	while (i < argc && strchr(argv[i], '=') != nil) {
		if (putenv(argv[i]) != 0) {
			fprintf(stderr, "env: Setting '%s' failed: %s\n",
					argv[i], strerror(errno));
			exit(EXIT_FAILURE);
		}
		++i;
	}

	/* Environment settings and command may be separated with '--'.
	 * This is for compatibility with other envs, we don't advertise it.
	 */
	if (i < argc && strcmp(argv[i], "--") == 0)
	  ++i;

	if (i >= argc) {
		/* No utility given; print environment. */
		char **ep;

		for (ep = environ; *ep != nil; ++ep) {
			if (puts(*ep) == EOF) {
				fprintf(stderr, "env: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		return 0;
	} else {
		char *util, **args;
		int err;

		util = argv[i];
		args = argv + i;
		if (aFlag)
		  ++args;
		execvp(util, args);
		err = errno;
		fprintf(stderr, "env: Can't execute %s: %s\n", util, strerror(errno));
		return err == ENOENT ? 127 : 126;
	}
}




