#define nil	0
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>

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

static int addPath(const char *path, char **ap, const char *entry) {
/* Add the name of a directory entry at the front of the path being built.
 * Note that the result always starts with a slash.
 */
	const char *e = entry;
	char *p = *ap;

	while (*e != 0) {
		++e;
	}

	while (e > entry && p > path) {
		*--p = *--e;
	}

	if (p == path)
	  return -1;
	*--p = '/';
	*ap = p;
	return 0;
}

char *getcwd(char *path, size_t size) {
	struct stat above, current, tmp;
	struct dirent *entry;
	DIR *dp;
	char *p, *up;
	const char *dotdot = "..";
	int cycle;

	if (path == nil || size <= 1) {
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
		cycle = (above.st_dev == current.st_dev ? 0 : 1);
		
		do {
			char name[3 + NAME_MAX + 1];	/* 3 for "../" */

			tmp.st_ino = 0;
			if ((entry = readdir(dp)) == nil) {
				switch (++cycle) {
					case 1:
						rewinddir(dp);
						continue;
					case 2:
						closedir(dp);
						errno = ENOENT;
						recover(p);
						return nil;
				}
			}
			if (strcmp(entry->d_name, ".") == 0 ||
				strcmp(entry->d_name, "..") == 0)
			  continue;

			switch (cycle) {
				case 0:
					/* Simple test on inode nr. */
					if (entry->d_ino != current.st_ino)
					  continue;
					/* Fall through */
				case 1:
					/* Current is mounted. */
					strcpy(name, "../");
					strcpy(name + 3, entry->d_name);
					if (stat(name, &tmp) < 0)
					  continue;
					break;
			}
		} while (tmp.st_ino != current.st_ino ||
					tmp.st_dev != current.st_dev);

		up = p;
		if (addPath(path, &up, entry->d_name) < 0) {
			closedir(dp);
			errno = ERANGE;
			recover(p);
			return nil;
		}
		closedir(dp);

		if (chdir(dotdot) < 0) {
			recover(p);
			return nil;
		}
		p = up;

		current = above;
	}
	if (recover(p) < 0)
	  return nil;		/* Undo all those chdir("..")'s. */
	if (*p == 0)
	  *--p = '/';		/* Cwd is "/" if nothing added */
	if (p > path)
	  strcpy(path, p);	/* Move string to start of path. */
	return path;
}
