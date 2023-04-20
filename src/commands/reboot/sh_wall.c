#define _POSIX_SOURCE	1
#include "sys/types.h"
#include "fcntl.h"
#include "pwd.h"
#include "string.h"
#include "stdio.h"
#include "time.h"
#include "utmp.h"
#include "unistd.h"
#include "sys/utsname.h"
#include "sys/stat.h"

static void crnlCat(char *message, char *more) {
	char *p = message;
	char *m = more;
	char *end = message + 1024 - 1;

	while (p < end && *p != 0) {
		++p;
	}
	while (p < end && *m != 0) {
		if (*m == '\n' && (p == message || p[-1] != '\n')) {
			*p++ = '\r';
			if (p == end)
			  --p;
		}
		*p++ = *m++;
	}
	*p = 0;
}

void wall(
	char *when,		/* When is shutdown */
	char *extra		/* If non-nil, why is the shutdown */
) {
	struct utmp utmp;
	char utmpTty[5 + sizeof(utmp.ut_line) + 1];
	char message[1024];
	struct passwd *pw;
	int utmpFd, ttyFd;
	char *ourTty, *ourName;
	time_t now;
	struct utsname utsName;
	struct stat conSt, ttySt;

	if ((ourTty = ttyname(STDIN_FILENO))) {
		if ((ourName = strrchr(ourTty, '/')))
		  ourTty = ourName + 1;
	} else {
		ourTty = "system task";
	}
	if ((pw = getpwuid(getuid())))
	  ourName = pw->pw_name;
	else
	  ourName = "unknown";

	time(&now);
	if (uname(&utsName) != 0)
	  strcpy(utsName.nodename, "?");
	sprintf(message, 
		"\r\nBroadcast message from %s@%s (%s)\r\n%.24s...\007\007\007\r\n",
		ourName, utsName.nodename, ourTty, ctime(&now));

	crnlCat(message, when);
	crnlCat(message, extra);

	/* Search the UTMP database for all logged-in users. */
	if ((utmpFd = open(UTMP, O_RDONLY)) < 0) {
		fprintf(stderr, "Cannot open utmp file\r\n");
		return;
	}

	/* First the console */
	strcpy(utmpTty, "/dev/console");
	if ((ttyFd = open(utmpTty, O_WRONLY | O_NONBLOCK)) < 0) {
		perror(utmpTty);
	} else {
		fstat(ttyFd, &conSt);
		write(ttyFd, message, strlen(message));
		close(ttyFd);
	}

	while (read(utmpFd, (char *) &utmp, sizeof(utmp)) == sizeof(utmp)) {
		/* Is this the user we are looking for? */
		if (utmp.ut_type != USER_PROCESS)
		  continue;
		strncpy(utmpTty + 5, utmp.ut_line, sizeof(utmp.ut_line));
		utmpTty[5 + sizeof(utmp.ut_line) + 1] = '\0';
		if ((ttyFd = open(utmpTty, O_WRONLY | O_NONBLOCK)) < 0) {
			perror(utmpTty);
			continue;
		}
		fstat(ttyFd, &ttySt);
		if (ttySt.st_rdev != conSt.st_rdev)
		  write(ttyFd, message, strlen(message));
		close(ttyFd);
	}
	close(utmpFd);
}



