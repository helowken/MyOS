#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "errno.h"
#include "stdio.h"
#include "ctype.h"
#include "unistd.h"
#include "stdlib.h"

static int _getTemp(char *template, register int *fdp) {
	extern int errno;
	register char *start, *trv;
	struct stat sb;
	pid_t pid;

	pid = getpid();
	for (trv = template; *trv; ++trv) {
		/* Extra X's get set to 0's */
	}
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/* Check the target directory; if you have six X's and it
	 * doesn't exist this runs for a *very* long time.
	 */
	for (start = trv + 1; ; --trv) {
		if (trv <= template)
		  break;
		if (*trv == '/') {
			*trv = '\0';
			if (stat(template, &sb) != 0)
			  return 0;
			if (! S_ISDIR(sb.st_mode)) {
				errno = ENOTDIR;
				return 0;
			}
			*trv = '/';
			break;
		}
	}

	for (;;) {
		if (fdp) {
			if ((*fdp = open(template, O_CREAT | O_EXCL | O_RDWR, 0600)) >= 0)
			  return 1;
			if (errno != EEXIST)
			  return 0;
		} else if (stat(template, &sb) != 0) {
			return errno == ENOENT ? 1 : 0;
		}

		/* Tricky little algorithm for backward compatibility.
		 *
		 * E.g,
		 *	 'a12345', 'b12345', 'c12345', ... , 'z12345',
		 *	 'aa2345', 'ba2345', 'ca2345', ... , 'za2345',
		 *	 'ab2345', 'bb2345', 'cb2345', ... , 'zb2345',
		 *	 ...									
		 *	 ...								 'zz2345',
		 *	 'aaa345', 'baa345', 'caa345', ... , 'zaa345',
		 *	 ...
		 */
		for (trv = start; ;) {
			if (! *trv) 
			  return 0;
			if (*trv == 'z')
			  *trv++ = 'a';
			else {
				if (isdigit(*trv))
				  *trv = 'a';
				else
				  ++*trv;
				break;
			}
		}
	}
}

int mkstemp(char *template) {
	int fd;

	return _getTemp(template, &fd) ? fd : -1;
}

char *mktemp(char *template) {
	return _getTemp(template, (int *) NULL) ? template : (char *) NULL;
}
