#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <minix/minlib.h>
#include <stdio.h>

#define S_IUGID	(S_ISUID | S_ISGID)

static int gFlag, oFlag, rFlag, error;
static char path[PATH_MAX + 1], *progName;
static uid_t nUid;
static gid_t nGid;

static void usageErr() {
	stdErr("Usage: ");
	stdErr(progName);
	stdErr(gFlag ? " owner[:group]" : " [owner:]group");
	stdErr( "file...\n");
	exit(EXIT_FAILURE);
}

static void doChown(char *file) {
	DIR *dirp;
	struct dirent *entp;
	char *namp;
	struct stat st;
	uid_t uid;
	gid_t gid;

	if (lstat(file, &st)) {
		perror(file);
		error = 1;
		return;
	}

	if (S_ISLNK(st.st_mode) && rFlag) {	/* Note: violates POSIX. */
		fprintf(stderr, "violates POSIX.\n");
		return;	
	}

	uid = oFlag ? nUid : st.st_uid;
	gid = gFlag ? nGid : st.st_gid;
	if (chown(file, uid, gid)) {
		perror(file);
		error = 1;
	}

	if (S_ISDIR(st.st_mode) && rFlag) {
		if ((dirp = opendir(file)) == NULL) {
			perror(file);
			error = 1;
			return;
		}
		if (path != file)
		  strcpy(path, file);
		namp = path + strlen(path);
		*namp++ = '/';
		while ((entp = readdir(dirp))) {
			if (entp->d_name[0] != '.' ||
				(entp->d_name[1] &&
					(entp->d_name[1] != '.' || entp->d_name[2]))) {
				strcpy(namp, entp->d_name);
				doChown(path);
			}
		}
		closedir(dirp);
		*--namp = '\0';
	}
}

int main(int argc, char **argv) {
	char *id, *id2;
	struct group *grp;
	struct passwd *pwd;
	int isChgrp;

	if ((progName = strrchr(*argv, '/')))
	  ++progName;
	else
	  progName = *argv;
	--argc;
	++argv;
	isChgrp = (strcmp(progName, "chgrp") == 0);

	if (argc && **argv == '-' && argv[0][1] == 'R') {
		--argc;
		++argv;
		rFlag = 1;
	}
	if (argc < 2) 
	  usageErr();
	
	id = *argv++;
	--argc;
	if ((id2 = strchr(id, ':')))
	  *id2++ = '\0';
	if (! id2 && isChgrp) {	/* chgrp */
		id2 = id;	/* group */
		id = 0;
	}
	if (id) {	/* owner exists */
		if (isdigit(*id)) {
			nUid = atoi(id);	
		} else {
			if (! (pwd = getpwnam(id))) {
				stdErr(id);
				stdErr(": unknown user name\n");
				exit(EXIT_FAILURE);
			}
			nUid = pwd->pw_uid;
		}
		oFlag = 1;
	} else {
		oFlag = 0;
	}

	if (id2) {	/* group exists */
		if (isdigit(*id2)) {
			nGid = atoi(id2);	
		} else {
			if (! (grp = getgrnam(id2))) {
				stdErr(id2);
				stdErr(": unknown group name\n");
				exit(EXIT_FAILURE);
			}
			nGid = grp->gr_gid;
		}
		gFlag = 1;
	} else {
		gFlag = 0;
	}

	errno = 0;
	while (argc--) {
		doChown(*argv++);
	}
	return errno;
}
