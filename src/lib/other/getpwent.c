#define nil	0
#include "sys/types.h"
#include "pwd.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"

#define arraySize(a)	(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)	((a) + arraySize(a))

static char PASSWD[] = "/etc/passwd";	/* The password file. */
static const char *pwFile;		/* Current password file. */

static char buf[1024];			/* Read buffer. */
static char pwLine[256];		/* One line from the password file. */
static struct passwd entry;		/* Entry to fill and return. */
static int pwFd = -1;			/* File descriptor to the file. */
static char *bufPtr;				
static size_t bufLen = 0;		/* Remaining characters in buf. */
static char *linePtr;

void endpwent() {
/* Close the password file. */
	if (pwFd > 0) {
		close(pwFd);
		pwFd = -1;
		bufLen = 0;
	}
}

int setpwent() {
/* Open the password file. */
	if (pwFd >= 0)
	  endpwent();

	if (pwFile == nil)
	  pwFile = PASSWD;

	if ((pwFd = open(pwFile, O_RDONLY)) < 0)
	  return -1;

	fcntl(pwFd, F_SETFD, fcntl(pwFd, F_GETFD) | FD_CLOEXEC);
	return 0;
}

void setpwfile(const char *file) {
/* Prepare for reading an alternate password file. */
	endpwent();
	pwFile = file;
}

static int getLine() {
/* Get one line from the password file, return 0 if bad or EOF. */
	linePtr = pwLine;

	do {
		if (bufLen == 0) {
			if ((bufLen = read(pwFd, buf, sizeof(buf))) <= 0)
			  return 0;
			bufPtr = buf;
		}
		if (linePtr == arrayLimit(pwLine))
		  return 0;
		--bufLen;
	} while ((*linePtr ++ = *bufPtr++) != '\n');

	linePtr = pwLine;
	return 1;
}

static char *scanColon() {
/* Scan for a field separator in a line, return the start of the field. */
	char *field = linePtr;
	char *last;

	for (;;) {
		last = linePtr;
		if (*linePtr == 0)
		  return nil;
		if (*linePtr == '\n')
		  break;
		if (*linePtr++ == ':')
		  break;
	}
	*last = 0;
	return field;
}

struct passwd *getpwent() {
/* Read one entry from the password file. */
	char *p;

	/* Open the file if mot yet open. */
	if (pwFd < 0 && setpwent() < 0)
	  return nil;

	/* Until a good line is read. */
	for (;;) {
		if (! getLine())
		  return nil;	/* EOF or corrupt. */

		if ((entry.pw_name = scanColon()) == nil ||
			(entry.pw_passwd = scanColon()) == nil)
		  continue;

		if ((p = scanColon()) == nil)
		  continue;
		entry.pw_uid = strtol(p, nil, 0);
		if ((p = scanColon()) == nil)
		  continue;
		entry.pw_gid = strtol(p, nil, 0);

		if ((entry.pw_gecos = scanColon()) == nil ||
			(entry.pw_dir = scanColon()) == nil ||
			(entry.pw_shell = scanColon()) == nil)
		  continue;

		if (*linePtr == 0)
		  return &entry;
	}
}

struct passwd *getpwuid(uid_t uid) {
/* Return the password file entry belonging to the user-id. */
	struct passwd *pw;

	endpwent();
	while ((pw = getpwent()) != nil && pw->pw_uid != uid) {
	}
	endpwent();
	return pw;
}

struct passwd *getpwnam(const char *name) {
/* Return the password file entry belonging to the user name. */
	struct passwd *pw;

	endpwent();
	while ((pw = getpwent()) != nil && 
				strcmp(pw->pw_name, name) != 0) {
	}
	endpwent();
	return pw;
}
