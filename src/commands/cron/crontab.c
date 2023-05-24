#define nil ((void *) 0)
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <limits.h>
#include <sys/stat.h>
#include "misc.h"
#include "tab.h"

#define seteuid(uid)

static void usage() {
	fprintf(stderr,
		"Usage: %s -c [user] file  # Change crontab\n"
		"       %s -l [user]       # List crontab \n"
		"       %s -r [user]       # Remove crontab \n"
		"       %s -p              # Tell cron to reload\n",
		progName, progName, progName, progName);
	exit(EXIT_FAILURE);
}

static int openTab(int uid, char *file, int how) {
/* Open a crontab file under the given uid. How is 'r' or 'w'. Return
 * the result of open(2).
 */
	//uid_t safeUid;
	int flags, r, err;

	switch (how) {
		case 'r': flags = O_RDONLY; break;
		case 'w': flags = O_WRONLY | O_CREAT | O_TRUNC; break;
		default:  errno = EINVAL;
	}
	//safeUid = geteuid();
	seteuid(uid);
	r = open(file, flags, 0666);
	err = errno;
	//seteuid(safeUid);
	errno = err;
	return r;
}

static void copyTab(int fdIn, char *fileIn, int fdOut, char *fileOut) {
/* Copy one open file to another. Complain and exit on errors. */
	size_t r, w;
	char buf[1024];

	while ((r = read(fdIn, buf, sizeof(buf))) > 0) {
		w = 0;
		while (w < r) {
			if ((r = write(fdOut, buf + w, r - w)) <= 0) {
				fprintf(stderr,
					"%s: Write error on %s: %s\n",
					progName, fileOut, 
					r == 0 ? "End of file" : strerror(errno));
				exit(EXIT_FAILURE);
			}
			w += r;
		}
	}
	if (r < 0) {
		fprintf(stderr, "%s: Read error on %s: %s\n",
			progName, fileIn, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv) {
	int i;
	int cFlag, lFlag, rFlag, pFlag;
	uid_t uid;
	char *user, *file;
	struct passwd *pw;
	static char SPOOL_DIR[] = "/usr/spool/crontabs";
	char tabFile[sizeof(SPOOL_DIR) + NAME_MAX];

	progName = strrchr(argv[0], '/');
	if (progName == nil)
	  progName = argv[0];
	else
	  ++progName;

	cFlag = lFlag = rFlag = pFlag = 0;
	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1;

		if (opt[0] == '-' && opt[1] == 0)	/* -- */
		  break;

		while (*opt != 0) {
			switch (*opt++) {
				case 'c': cFlag = 1; break;
				case 'l': lFlag = 1; break;
				case 'r': rFlag = 1; break;
				case 'p': pFlag = 1; break;
				default: usage();
			}
		}
	}
	if (cFlag + lFlag + rFlag + pFlag != 1)
	  usage();

	user = file = nil;
	if (! pFlag && i < argc)
	  user = argv[i++];
	if (cFlag) {
		if (user == nil)
		  usage();
		if (i < argc) {
			file = argv[i++];
		} else {
			file = user;
			user = nil;
		}
	}
	if (i != argc)
	  usage();

	if (geteuid() != 0) 
	  fprintf(stderr, "%s: No root privileges?\n", progName);

	uid = getuid();
	if (user == nil) {
		if ((pw = getpwuid(uid)) == nil) {
			fprintf(stderr, "%s: Don't know who you (uid %lu) are!\n",
					progName, (unsigned long) uid);
			exit(EXIT_FAILURE);
		}
	} else {
		if ((pw = getpwnam(user)) == nil) {
			fprintf(stderr, "%s: Don't know who you (%s) are!\n",
					progName, user);
			exit(EXIT_FAILURE);
		}
	}
	if (uid != 0 && pw->pw_uid != uid) {
		fprintf(stderr,
				"%s: Only root can change the crontabs of others!\n",
				progName);
		exit(EXIT_FAILURE);
	}
	user = pw->pw_name;
	uid = pw->pw_uid;
	seteuid(uid);
	umask(0077);

	selectLog(STDERR);
	sprintf(tabFile, "%s/%s", SPOOL_DIR, user);

	if (lFlag) {
		int fd;

		if ((fd = openTab(0, tabFile, 'r')) < 0) {
			fprintf(stderr, "%s: Can't open %s: %s\n",
				progName, tabFile, strerror(errno));
			exit(EXIT_FAILURE);
		}
		copyTab(fd, tabFile, 1, "stdout");
		close(fd);
	}

	if (rFlag) {
		seteuid(0);
		if (unlink(tabFile) < 0) {
			fprintf(stderr, "%s: Can't remove %s: %s\n",
				progName, tabFile, strerror(errno));
			exit(EXIT_FAILURE);
		}
		seteuid(uid);
		printf("Crontab of %s removed\n", user);
		pFlag = 1;
	}

	/* Initialize current time */
	time(&now);

	if (cFlag) {
		int fd1, fd2;
		
		if ((fd1 = openTab(uid, file, 'r')) < 0) {
			fprintf(stderr, "%s: Can't open %s: %s\n",
				progName, file, strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Try to parse the new crontab file. If the parsing
		 * succeeds then 'crontabs' will be non-null.
		 */
		tabParse(file, user);
		tabPurge();
		if (crontabs == nil)
		  exit(EXIT_FAILURE);

		if ((fd2 = openTab(0, tabFile, 'w')) < 0) {
			fprintf(stderr, "%s: Can't open %s: %s\n",
				progName, tabFile, strerror(errno));
			exit(EXIT_FAILURE);
		}
		copyTab(fd1, file, fd2, tabFile);
		close(fd1);
		close(fd2);
		printf("New crontab for %s installed\n", user);
		pFlag = 1;
	}

	if (pFlag) {
		/* Alert cron to the new situation. */
		FILE *fp;

		seteuid(0);
		if ((fp = fopen(PID_FILE, "r")) != NULL) {
			unsigned long pid;
			int c;

			pid = 0;
			while ((c = fgetc(fp)) != EOF && c != '\n') {
				if ((unsigned) c - '0' >= 10) {
					pid = 0;
					break;
				}
				pid = 10 * pid + (c - '0');
				if (pid >= 30000) {
					pid = 0;
					break;
				}
			}
			if (pid > 1 && kill((pid_t) pid, SIGHUP) == 0) {
				pFlag = 0;
			}
		}
		seteuid(uid);
		if (pFlag) {
			fprintf(stderr,
				"%s: Alerting cron has failed; cron still running?\n",
				progName);
			exit(EXIT_FAILURE);
		}
		printf("Cron signalled to reload tables\n");
	}
	return EXIT_SUCCESS;
}


