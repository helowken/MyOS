#if defined(_POSIX_SOURCE)
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "loc_incl.h"

#define PMODE	0666

/* Do not "optimize" this file to use the open with O_CREAT if the file
 * does not exist. The reason is given in fopen.c
 */
#define O_RDONLY	0
#define O_WRONLY	1
#define O_RDWR		2

#define O_CREAT		0x010
#define O_TRUNC		0x020
#define O_APPEND	0x040

int open(const char *path, int flags);
int creat(const char *path, mode_t mode);
int close(int fd);

FILE *freopen(const char *path, const char *mode, FILE *stream) {
	register int i;
	struct stat st;
	int rwMode = 0, rwFlags = 0;
	int fd;
	int flags = stream->_flags & (_IONBF | _IOFBF | _IOLBF | _IOMYBUF);

	fflush(stream);	
	close(fileno(stream));

	switch(*mode++) {
		case 'r':
			flags |= _IOREAD;
			rwMode = O_RDONLY;
			break;
		case 'w':
			flags |= _IOWRITE;
			rwMode = O_WRONLY;
			rwFlags = O_CREAT | O_TRUNC;
			break;
		case 'a':
			flags |= _IOWRITE | _IOAPPEND;
			rwMode = O_WRONLY;
			rwFlags |= O_APPEND | O_CREAT;
			break;
		default:
			goto loser;
	}

	while (*mode) {
		switch (*mode++) {
			case 'b':
				continue;
			case '+':
				rwMode = O_RDWR;
				flags |= _IOREAD | _IOWRITE;
				continue;
			/* The sequence may be followed by additional characters */
			default:
				break;
		}
		break;
	}

	if ((rwFlags & O_TRUNC) ||
				((fd = open(path, rwMode)) < 0 && (rwFlags & O_CREAT))) {
		if ((fd = creat(path, PMODE)) > 0 && (flags & _IOREAD)) {
			close(fd);
			fd = open(path, rwMode);
		}
	}

	if (fd < 0)
	  goto loser;

	if (fstat(fd, &st) == 0) {
		if (st.st_mode & S_IFIFO)
		  flags |= _IOFIFO;
	} else {
		goto loser;
	}

	stream->_count = 0;
	stream->_fd = fd;
	stream->_flags = flags;
	return stream;

loser:
	for (i = 0; i < FOPEN_MAX; ++i) {
		if (stream == __iotab[i]) {
			__iotab[i] = 0;
			break;
		}
	}
	if (stream != stdin && stream != stdout && stream != stderr)
	  free((FILE *) stream);
	return (FILE *) NULL;
}
