#include "dirent.h"
#include "chmod.h"

static int rflag, errors;
static mode_t newMode;
static char path[PATH_MAX + 1];

void usageErr() {
	usage(prog, "[-R] mode file...");
}

/* Apply a mode change to a given file system element. */
int doChange(char *name) {
	mode_t m;
	DIR *dp;
	struct dirent *ep;
	char *np;

	if (lstat(name, &st)) {
		perror(name);
		return 1;
	}
	if (S_ISLNK(st.st_mode) && rflag)
	  return 0;		/* Note: violates POSIX. */
	if (! _symbolic) 
	  m = newMode;
	else
	  m = doParseMode(_symbolic, st.st_mode);

	if (chmod(name, m)) {
		perror(name);
		errors = 1;
	} else
	  errors = 0;

	if (S_ISDIR(st.st_mode) && rflag) {
		if ((dp = opendir(name)) == nil) {
			perror(name);
			return 1;
		}
		if (name != path)
		  strcpy(path, name);
		np = path + strlen(path);
		*np++ = '/';
		while ((ep = readdir(dp)) != nil) {
			if (ep->d_name[0] != '.' ||
					(ep->d_name[1] && 
						(ep->d_name[1] != '.' || ep->d_name[2]))) {
				strcpy(np, ep->d_name);
				errors |= doChange(path);
			}
		}
		closedir(dp);
		*--np = '\0';
	}
	return errors;
}

int main(int argc, char **argv) {
	int exitCode = 0;

	prog = getProg(argv);
	--argc;
	++argv;

	if (argc && strcmp(*argv, "-R") == 0) {
		--argc;
		++argv;
		rflag = 1;
	} else {
		rflag = 0;
	}

	if (! argc--)
	  usageErr();
	if (! strcmp(argv[0], "--")) {	/* Allow chmod -- -r, as in Draft11 example */
		if (! argc--)
		  usageErr();
		++argv;
	}
	_symbolic = *argv++;
	if (! argc)
	  usageErr();

	if (getOctalMode(_symbolic, &newMode) == -1) 
	  u_mask = umask(0);
	else 
	  _symbolic = nil;

	while (argc--) {
		if (doChange(*argv++))
		  exitCode = 1;
	}
	return exitCode;
}
