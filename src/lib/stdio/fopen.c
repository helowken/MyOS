#if defined(_POSIX_SOURCE)
#include "sys/types.h"
#endif

#include "stdio.h"
#include "stdlib.h"
#include "loc_incl.h"
#include "sys/stat.h"

#define PMODE	0666

/* The next 3 defines are true in all UNIX systems known to me.
 */
#define O_RDONLY	0
#define O_WRONLY	1
#define O_RDWR		2

/* Since the O_CREAT flag is not available on all systems, we can't get it
 * from the standard library. Furthermore, even if we know that "fcntl.h"
 * contains such a flag, it's not sure whether it can be used, since we
 * might be cross-compiling for another system, which may use an entirely
 * different value for O_CREAT (or not support such a mode). The safest
 * thing is to just use the Version 7 semantics for open, and use creat()
 * whenever necessary.
 *
 * Another problem is O_APPEND, for which the same holds. When "a"
 * open-mode is used, an lseek() to the end is done before every write()
 * system-call.
 *
 * The O_CREAT, O_TRUNC and O_APPEND given here, are only for convenience.
 * They are not passed to open(), so the values don't have to match a value
 * from he real world. It is enough when they are unique.
 */
#define O_CREAT		0x010
#define O_TRUNC		0x020
#define O_APPEND	0x040

int open(const char *path, int flags);
int creat(const char *path, mode_t mode);
int close(int fd);

FILE *fopen(const char *path, const char *mode) {
	register int i;

	int rwMode = 0, rwFlags = 0;
	FILE *stream;
	struct stat st;
	int fd, flags = 0;

	for (i = 0; __iotab[i] != 0; ++i) {
		if (i >= FOPEN_MAX - 1)
		  return (FILE *) NULL;
	}

	switch (*mode++) {
		case 'r':
			flags |= _IOREAD | _IOREADING;
			rwMode = O_RDONLY;
			break;
		case 'w':
			flags |= _IOWRITE | _IOWRITING;
			rwMode = O_WRONLY;
			rwFlags = O_CREAT | O_TRUNC;
			break;
		case 'a':
			flags |= _IOWRITE | _IOWRITING | _IOAPPEND;
			rwMode = O_WRONLY;
			rwFlags |= O_APPEND | O_CREAT;
			break;
		default:
			return (FILE *) NULL;
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
	}

	/* Perform a creat() when the file should be truncated or when
	 * the file is opened for writing and the open() failed.
	 */
	if ((rwFlags & O_TRUNC) ||
				((fd = open(path, rwMode)) < 0 && (rwFlags & O_CREAT))) {
		if ((fd = creat(path, PMODE)) > 0 && (flags & _IOREAD)) {
			close(fd);
			fd = open(path, rwMode);
		}
	}
	
	if (fd < 0)
	  return (FILE *) NULL;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return (FILE *) NULL;
	}

	if (st.st_mode & S_IFIFO)
	  flags |= _IOFIFO;

	if ((stream = (FILE *) malloc(sizeof(FILE))) == NULL) {
		close(fd);
		return (FILE *) NULL;
	}

	if ((flags & (_IOREAD | _IOWRITE)) == (_IOREAD | _IOWRITE))
	  flags &= ~(_IOREADING | _IOWRITING);

	stream->_count = 0;
	stream->_fd = fd;
	stream->_flags = flags;
	stream->_buf = NULL;
	__iotab[i] = stream;
	return stream;
}
