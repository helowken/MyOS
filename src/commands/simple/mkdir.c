#include "chmod.h"
#define DEFAULT_MODE	(S_IRWXU | S_IRWXG | S_IRWXO)
#define USER_WX		(S_IWUSR | S_IXUSR)

static int pflag;

static void usageErr() {
	usage(prog, "[-p] [-m mode] dir...");
}

static int makePath(char *forDir) {
/* P1003.2 requires that missing intermediate pathname components should be
 * created if the -p option is specified (4.40.3).
 */
	char parent[PATH_MAX + 1], *end, *last;

	strcpy(parent, forDir);
	/* For forDir = "/a/b/c/./d", parent = "/a/b/c" */
	do {
		if (! (end = strrchr(parent, '/')))
		  return 0;
		*end = 0;
		if (parent[0] == 0 || strcmp(parent, ".") == 0)
		  return 0;
	} while ((last = strrchr(parent, '/')) && 
					(strcmp(last + 1, ".") == 0 || *(last + 1) == 0));

	if (! stat(parent, &st)) {	/* Error if parent is not a dir */
		if (S_ISDIR(st.st_mode))	
		  return 0;
		errno = ENOTDIR;
		perror(parent);
		return 1;
	}
	if (mkdir(parent, DEFAULT_MODE)) {	/* "mkdir parent" failed */
		if (makePath(parent))	/* Error if "mkdir -p parent" failed */
		  return 1;
		if (mkdir(parent, DEFAULT_MODE)) {	/* Error if try again and failed */
			perror(parent);
			return 1;
		}
	}

	/* P1003.2 states that, regardless of umask() value, intermediate paths
	 * should have at least write and search (x) permissions (4.40.10).
	 */
	if ((u_mask & USER_WX) && 
			chmod(parent, (((~u_mask) | USER_WX) & DEFAULT_MODE))) {
		perror(parent);
		return 1;
	}
	return 0;
}

static mode_t parseMode(char *symbolic, mode_t oldMode) {
	mode_t newMode;

	if (getOctalMode(symbolic, &newMode) == -1) 
	  newMode = doParseMode(symbolic, oldMode);
	return newMode;
}

static int makeDir(char *dirName) {
/* Actual directory creation, using a mkdir() system call. */
	if (mkdir(dirName, DEFAULT_MODE)) {	/* mkdir failed */
		if (! pflag) {			/* Error if without -p */
			perror(dirName);
			return 1;
		}
		if (! stat(dirName, &st)) {	/* Error if not dir */
			if (S_ISDIR(st.st_mode))
			  return 0;
			errno = ENOTDIR;
			perror(dirName);
			return 1;
		}
		if (makePath(dirName))	/* try for "mkdir -p" */
		  return 1;
		if (mkdir(dirName, DEFAULT_MODE)) {	/* Error if try again and failed*/
			perror(dirName);
			return 1;
		}
	}
	if (_symbolic && 
			(stat(dirName, &st) ||	/* stat failed */
				chmod(dirName, parseMode(_symbolic, st.st_mode)))) { /* chmod failed */
		perror(dirName);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv) {
	int error, c;

	prog = argv[0];
	opterr = 0;
	pflag = 0;
	_symbolic = nil;
	u_mask = umask(0);
	umask(u_mask);
	while ((c = getopt(argc, argv, "m:p")) != EOF) {
		switch (c) {
			case 'm':
				_symbolic = optarg;
				break;
			case 'p':
				pflag = 1;
				break;
			default:
				usageErr();
		}
	}
	if (optind >= argc)
	  usageErr();
	
	error = 0;
	while (optind < argc) {
		error |= makeDir(argv[optind++]);
	}
	return error;
}
