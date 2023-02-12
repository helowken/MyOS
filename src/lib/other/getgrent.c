#define nil	0
#include "sys/types.h"
#include "grp.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"

#define arraySize(a)	(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)	((a) + arraySize(a))

static char GROUP[] = "/etc/group";		/* The group file. */
static const char *grFile;		/* Current group file. */

static char buf[1024];			/* Read buffer. */
static char grLine[512];		/* One line from the group file. */
static struct group entry;		/* Entry to fill and return. */
static char *members[64];		/* Group members with the entry */
static int grFd = -1;			/* File descriptor to the file. */
static char *bufPtr;				
static size_t bufLen = 0;		/* Remaining characters in buf. */
static char *linePtr;

void endgrent() {
/* Close the group file. */
	if (grFd > 0) {
		close(grFd);
		grFd = -1;
		bufLen = 0;
	}
}

int setgrent() {
/* Open the group file. */
	if (grFd >= 0)
	  endgrent();

	if (grFile == nil)
	  grFile = GROUP;

	if ((grFd = open(grFile, O_RDONLY)) < 0)
	  return -1;

	fcntl(grFd, F_SETFD, fcntl(grFd, F_GETFD) | FD_CLOEXEC);
	return 0;
}

void setgrfile(const char *file) {
/* Prepare for reading an alternate group file. */
	endgrent();
	grFile = file;
}

static int getLine() {
/* Get one line from the group file, return 0 if bad or EOF. */
	linePtr = grLine;

	do {
		if (bufLen == 0) {
			if ((bufLen = read(grFd, buf, sizeof(buf))) <= 0)
			  return 0;
			bufPtr = buf;
		}
		if (linePtr == arrayLimit(grLine))
		  return 0;
		--bufLen;
	} while ((*linePtr ++ = *bufPtr++) != '\n');

	linePtr = grLine;
	return 1;
}

static char *scanPunct(int punct) {
/* Scan for a field separator in a line, return the start of the field. */
	char *field = linePtr;
	char *last;

	for (;;) {
		last = linePtr;
		if (*linePtr == 0)
		  return nil;
		if (*linePtr == '\n')
		  break;
		if (*linePtr++ == punct)
		  break;
		if (linePtr[-1] == ':')
		  return nil;	/* :::,,,:,,,? */
	}
	*last = 0;
	return field;
}

struct group *getgrent() {
/* Read one entry from the group file. */
	char *p;
	char **mem;

	/* Open the file if mot yet open. */
	if (grFd < 0 && setgrent() < 0)
	  return nil;

	/* Until a good line is read. */
	for (;;) {
		if (! getLine())
		  return nil;	/* EOF or corrupt. */

		if ((entry.gr_name = scanPunct(':')) == nil ||
			(entry.gr_passwd = scanPunct(':')) == nil)
		  continue;

		if ((p = scanPunct(':')) == nil)
		  continue;
		entry.gr_gid = strtol(p, nil, 0);

		entry.gr_mem = mem = members;
		if (*linePtr != '\n') {
			do {
				if ((*mem = scanPunct(',')) == nil)
				  goto again;
				if (mem < arrayLimit(members) - 1)
				  ++mem;
			} while (*linePtr != 0);
		}
		*mem = nil;
		return &entry;
again:;
	}
}

struct group *getgrgid(gid_t gid) {
/* Return the group file entry belonging to the group-id. */
	struct group *gr;

	endgrent();
	while ((gr = getgrent()) != nil && gr->gr_gid != gid) {
	}
	endgrent();
	return gr;
}

struct group *getgrnam(const char *name) {
/* Return the group file entry belonging to the group name. */
	struct group *gr;

	endgrent();
	while ((gr = getgrent()) != nil && 
				strcmp(gr->gr_name, name) != 0) {
	}
	endgrent();
	return gr;
}


