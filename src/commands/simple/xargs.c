#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdarg.h>

#define _PATH_ECHO	"/bin/echo"

static int tFlag;
static int exitStatus = 0;

static void err(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "xargs: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

static void usage() {
	fprintf(stderr,
"usage: xargs [-ft0] [[-x] -n number] [-s size] [utility [argument ...]]\n");
	exit(1);
}

static void run(char **argv) {
	register char **p;
	pid_t pid;
	int noInvoke;
	int status;
	int pfd[2];

	if (tFlag) {
		fprintf(stderr, "%s", *argv);
		for (p = argv + 1; *p; ++p) {
			fprintf(stderr, " %s", *p);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	if (pipe(pfd) < 0)
	  err("pipe: %s", strerror(errno));

	switch ((pid = fork())) {
		case -1:
			err("fork: %s", strerror(errno));
		case 0:		/* Child */
			close(pfd[0]);
			fcntl(pfd[1], F_SETFD, fcntl(pfd[1], F_GETFD) | FD_CLOEXEC);

			execvp(argv[0], argv);
			noInvoke = (errno == ENOENT ? 127 : 126);	/* See exit status in Linux man */
			fprintf(stderr, "xargs: %s: %s.\n", argv[0], strerror(errno));

			write(pfd[1], &noInvoke, sizeof(noInvoke));
			_exit(-1);
	}
	close(pfd[1]);
	if (read(pfd[0], &noInvoke, sizeof(noInvoke)) < sizeof(noInvoke))
	  noInvoke = 0;
	close(pfd[0]);

	pid = waitpid(pid, &status, 0);
	if (pid == -1)
	  err("waitpid: %s", strerror(errno));

	/* If we couldn't invoke the utility or the utility didn't exit
	 * properly, quit with 127 or 126 respectively.
	 */
	if (noInvoke)
	  exit(noInvoke);

	/* According to POSIX, we have to exit if the utility exits with
	 * a 255 status, or is interrupted by a signal.  xargs is allowed
	 * to return any exit status between 1 and 125 in these cases, but
	 * we'll use 124 and 125, the same values used by GNU xargs.
	 *
	 * See exit status in Linux man.
	 */
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 255) {
			fprintf(stderr, "xargs: %s exited with status 255\n", argv[0]);
			exit(124);
		} else if (WEXITSTATUS(status) != 0) {
			exitStatus = 123;
		}
	} else if (WIFSTOPPED(status)) {
		fprintf(stderr, "xargs: %s terminated by signal %d\n", argv[0], WSTOPSIG(status));
		exit(125);
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr, "xargs: %s terminated by signal %d\n", argv[0], WTERMSIG(status));
		exit(125);
	}
}

void main(int argc, char **argv) {
	register int ch;
	register char *p, *bbp, *ebp, **bxp, **exp, **xp;
	int cnt, inDouble, inSingle, nArgs, nLine, nFlag, xFlag, zFlag;
	char **av, *argp;

	/* POSIX.2 limits the exec line length to ARG_MAX - 2K. Running that
	 * caused some E2BIG errors, so it was changed to ARG_MAX - 4K. Given
	 * that the smallest argument is 2 bytes in length, this means that
	 * the number of arguments is limited to:
	 *
	 *   (ARG_MAX - 4K - LENGTH(utility + arguments)) / 2.
	 *
	 * We arbitrarily limit the number of arguments to 5000. This is
	 * allowed by POSIX.2 as long as the resulting minumem exec line is
	 * at least LINE_MAX. Realloc'ing as necessary is possible, but
	 * probably not worthwhile.
	 */
	nArgs = 5000;
	nLine = ARG_MAX - 4 * 1024;
	nFlag = xFlag = zFlag = 0;
	while ((ch = getopt(argc, argv, "n:s:tx0")) != EOF) {
		switch (ch) {
			case 'n':
				nFlag = 1;
				if ((nArgs = atoi(optarg)) <= 0)
				  err("illegal argument count");
				break;
			case 's':
				nLine = atoi(optarg);
				break;
			case 't':
				tFlag = 1;
				break;
			case 'x':
				xFlag = 1;
				break;
			case '0':
				zFlag = 1;
				break;
			case '?':
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (xFlag && ! nFlag)
	  usage();

	/* Allocate pointers for the utility name, the utility arguments,
	 * the maximum arguments to be read from stdin and the trailing
	 * NULL.
	 */
	if (! (av = bxp = malloc((u_int) (1 + argc + nArgs + 1) * sizeof(char **))))
	  err("%s", strerror(errno));

	/* Use the user's name for the utility as argv[0], just like the
	 * shell. Echo is the default. Set up pointers for the user's
	 * arguments.
	 */
	if (! *argv) {
		cnt = strlen((*bxp++ = _PATH_ECHO));
	} else {
		cnt = 0;
		do {
			cnt += strlen((*bxp++ = *argv)) + 1;
		} while (*++argv);
	}

	/* Set up begin/end/traversing pointers into the array. The -n
	 * count doesn't include the trailing NULL pointer, so the malloc
	 * added in an extra slot.
	 */
	xp = bxp;
	exp = bxp + nArgs;

	/* Allocate buffer space for the arguments read from stdin and the
	 * trailing NULL. Buffer space is defined as the default or specified
	 * space, minus the length of the utility name and arguments. Set up
	 * begin/end/traversing pointers into the array. The -s count does
	 * include the trailing NULL, so the malloc didn't add in an extra
	 * slot.
	 */
	nLine -= cnt;
	if (nLine <= 0)
	  err("insufficient space for command");

	if ((bbp = malloc((u_int) nLine + 1)) == NULL)
	  err("%s", strerror(errno));
	argp = p = bbp;
	ebp = bbp + nLine - 1;

	if (zFlag) {
		/* Read pathnames terminated by null bytes as produced by
		 * find ... -print0. No comments in this code, see further
		 * below.
		 */
		for(;;) {
			switch ((ch = getchar())) {
				case EOF:
					if (p == bbp)	/* No input arguments */
					  exit(exitStatus);

					if (p == argp) {	/* End of argument list */
						*xp = NULL;
						run(av);
						exit(exitStatus);
					}
					/* FALL THROUGH */
				case '\0':
					if (p == argp)	/* Skip null arguments */
					  continue;

					*p = '\0';		/* End of current argument */
					*xp++ = argp;	/* Append current argument to argument list */

					if (xp == exp || p == ebp || ch == EOF) {
						if (xFlag && xp != exp && p == ebp)
						  err("insufficient space for arguments");
						/* Consume arguments since now maybe:
						 * 1. argument list is full or
						 * 2. buffer is full or
						 * 3. EOF
						 */
						*xp = NULL;
						run(av);
						if (ch == EOF)
						  exit(exitStatus);
						p = bbp;
						xp = bxp;
					} else {
						++p;
					}
					argp = p;	/* Start a new argument */
					break;
				default:
					if (p < ebp) {
						*p++ = ch;
						break;
					}

					if (xp == bxp)	/* The argument is too large */
					  err("insufficient space for argument");
					if (xFlag)		/* Buffer cannot contain all the arguments */
					  err("insufficient space for arguments");

					/* Consume arguments, since buffer is not large enough to contain all of them */
					*xp = NULL;
					run(av);

					/* After consuming the previous arguments, copy the current one to the head of the buffer */
					xp = bxp;
					cnt = ebp - argp;
					bcopy(argp, bbp, cnt);
					argp = bbp;
					p = bbp + cnt;
					*p++ = ch;
					break;
			}
		}
	}

	for (inSingle = inDouble = 0;;) {
		switch ((ch = getchar())) {
			case EOF:
				/* No arguments since last exec. */
				if (p == bbp)
				  exit(exitStatus);

				/* Nothing since end of last argument. */
				if (p == argp) {
					*xp = NULL;
					run(av);
					exit(exitStatus);
				}
				goto arg1;
			case ' ':
			case '\t':
				/* Quotes escape tabs and spaces. */
				if (inSingle || inDouble)
				  goto addCh;
				goto arg2;
			case '\n':
				/* Empty lines are skipped. */
				if (p == argp)
				  continue;

				/* Quotes do not escape newlines. */
arg1:		
				if (inSingle || inDouble)
				  err("unterminated quote");
arg2:
				*p = '\0';
				*xp++ = argp;

				/* If max'd out on args or buffer, or reached EOF,
				 * run the command. If xFlag and max'd out on buffer
				 * but not on args, object.
				 */
				if (xp == exp || p == ebp || ch == EOF) {
					if (xFlag && xp != exp && p == ebp)
					  err("insufficient space for arguments");
					*xp = NULL;
					run(av);
					if (ch == EOF)
					  exit(exitStatus);
					p = bbp;
					xp = bxp;
				} else {
					++p;
				}
				argp = p;
				break;
			case '\'':
				if (inDouble)
				  goto addCh;
				inSingle = ! inSingle;
				break;
			case '"':
				if (inSingle)
				  goto addCh;
				inDouble = ! inDouble;
				break;
			case '\\':
				/* Backslash escapes anything, is escaped by quotes. */
				if (! inSingle && ! inDouble && (ch = getchar()) == EOF)
				  err("backslash at EOF");
				/* FALL THROUGH */
			default:
addCh:
				if (p < ebp) {
					*p++ = ch;
					break;
				}

				/* If only one argument, not enough buffer space. */
				if (xp == bxp)
				  err("insufficient space for argument");
				/* Didn't hit argument limit, so if xFlag object. */
				if (xFlag)
				  err("insufficient space for arguments");

				*xp = NULL;
				run(av); 
				xp = bxp;
				cnt = ebp - argp;
				bcopy(argp, bbp, cnt);
				argp = bbp;
				p = bbp + cnt;
				*p++ = ch;
				break;
		}
	}
}


