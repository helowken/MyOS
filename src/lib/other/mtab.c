/* This package consists of 4 routines for handling the /etc/mtab file.
 * The /etc/mtab file contains information about the root and mounted file
 * systems as a series of lines, each one with exactly four fields separated
 * by one space as follows:
 *
 *	special mountedOn version rwFlag
 *
 * where
 *	special is the name of the block special file
 *	mountedOn is the directory on which it is mounted
 *	version is either 1 or 2 for MINIX file systems
 *	rwFlag is rw or ro for read/write or read only
 *
 */

#include "sys/types.h"
#include "minix/minlib.h"
#include "ctype.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "stdio.h"

#define BUF_SIZE	512		/* Size of the /etc/mtab buffer */

static char *mtabPath = "/etc/mtab";	
static char inBuf[BUF_SIZE + 1];	/* Holds /etc/mtab when it is read in */
static char outBuf[BUF_SIZE + 1];	/* Buf to build /etc/mtab for output later */
static char *inPtr = inBuf;			/* Pointer to next line to feed out */
static char *outPtr = outBuf;		/* Pointer to place where next line goes */

static void err(char *prog, char *s) {
	stdErr(prog);
	stdErr(s);
	stdErr(mtabPath);
	perror(" ");
}

int loadMTab(char *prog) {
/* Read in /etc/mtab and store it in /etc/mtab. */
	int fd, n;
	char *ptr;

	fd = open(mtabPath, O_RDONLY);
	if (fd < 0) {
		err(prog, ": cannot open ");
		return -1;
	}

	n = read(fd, inBuf, BUF_SIZE);
	if (n <= 0) {
		err(prog, ": cannot read ");
		return -1;
	}
	if (n == BUF_SIZE) {
		/* Some nut has mounted 50 file systems or something like that. */
		stdErr(prog);
		stdErr(": file too large: ");
		stdErr(mtabPath);
		return -1;
	}
	close(fd);

	/* Replace all the whitespace by '\0'. */
	ptr = inBuf;
	while (*ptr != '\0') {
		if (isspace(*ptr))
		  *ptr = '\0';
		++ptr;
	}
	return 0;
}

int rewriteMTab(char *prog) {
/* Write outBuf to /etc/mtab. */
	int fd, n;

	/* Do a create to truncate the file. */
	fd = creat(mtabPath, 0777);
	if (fd < 0) {
		err(prog, ": cannot overwrite ");
		return -1;
	}

	/* File created. Write it. */
	n = write(fd, outBuf, (unsigned int) (outPtr - outBuf));
	if (n <= 0) {
		err(prog, " could not write ");
		return -1;
	}
	close(fd);
	return 0;
}

static void readItem(char *s) {
	strcpy(s, inPtr);
	while(isprint(*inPtr)) {
		++inPtr;
	}
	while (*inPtr == '\0' && inPtr < &inBuf[BUF_SIZE]) {
		++inPtr;
	}
}

int getMTabEntry(char *special, char *mountedOn, char *version, 
			char *rwFlag) {
/* Return the next entry from inBuf. */

	if (inPtr >= &inBuf[BUF_SIZE]) {
		special[0] = '\0';
		return -1;
	}
	readItem(special);
	readItem(mountedOn);
	readItem(version);
	readItem(rwFlag);
	return 0;
}

static void writeItem(char *s, int n, char c) {
	strcpy(outPtr, s);
	outPtr += n;
	*outPtr++ = c;
}

int putMTabEntry(char *special, char *mountedOn, char *version, 
			char *rwFlag) {
/* Append an entry to the outBuf buffer. */
	int n1, n2, n3, n4;

	n1 = strlen(special);
	n2 = strlen(mountedOn);
	n3 = strlen(version);
	n4 = strlen(rwFlag);

	if (outPtr + n1 + n2 + n3 + n4 + 5 >= &outBuf[BUF_SIZE])
	  return -1;

	writeItem(special, n1, ' ');
	writeItem(mountedOn, n2, ' ');
	writeItem(version, n3, ' ');
	writeItem(rwFlag, n4, '\n');
	return 0;
}

