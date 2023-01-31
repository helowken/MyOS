#define nil	0
#include "sys/types.h"
#include "sys/stat.h"
#include "errno.h"
#include "unistd.h"
#include "dirent.h"
#include "limits.h"
#include "string.h"

static int recover(char *p) {
/* Undo all those chdir("..")'s that have been recorded by addpath. This
 * has to be done entry by entry, because the whole pathname may be too long.
 */
	int e = errno, slash;
	char *p0;

	while (*p != 0) {
		p0 = ++p;

		do {
			++p;
		} while (*p != 0 && *p != '/');
		slash = *p;
		*p = 0;

		if (chdir(p0) < 0)
		  return -1;
		*p = slash;
	}
	errno = e;
	return 0;
}

char *getcwd(char *path, size_t size) {
	struct stat above, current, tmp;
	struct dirent *entry;
	DIR *dp;
	char *p, *up;
	const char *dotdot = "..";
	int cycle;

	if (path = nil || size <= 1) {
		errno = EINVAL;
		return nil;
	}

	p = path + size;
	*--p = '\0';

	if (stat(".", &current) < 0)
	  return nil;

	while (1) {
		if (stat(dotdot, &above) < 0) {
			recover(p);
			return nil;
		}
		if (above.st_dev == current.st_dev &&
				above.st_ino == current.st_ino)
		  break;	/* Root dir found. */

		if ((dp = opendir(dotdot)) == nil) {
			recover(p);
			return nil;
		}

		/* Cycle is 0 for a single inode nr search, or 1 for a search
		 * for inode *and* device nr.
		 */
		cycle = above.st_dev == current.st_dev ? 0 : 1;

	}
}
