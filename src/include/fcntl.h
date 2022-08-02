#ifndef _FCNTL_H
#define _FCNTL_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* These values are used for cmd in fcntl(). */
#define	F_DUPFD			0	/* Duplicate file descriptor */
#define F_GETFD			1	/* Get file descriptor flags */
#define F_SETFD			2	/* Set file descriptor flags */
#define F_GETFL			3	/* Get file status flags */
#define F_SETFL			4	/* Set file status flags */
#define F_GETLK			5	/* Get record locking information */
#define F_SETLK			6	/* Set record locking information */
#define F_SETLKW		7	/* Set record locking info; wait if blocked */

/* Oflag values for open() */
#define O_CREAT		00100	/* Create file if it doesn't exist */
#define O_EXCL		00200	/* Exclusive use flag */
#define O_NOCTTY	00400	/* Do not assign a controlling terminal */
#define O_TRUNC		01000	/* Truncate flag */

/* File status flags for open() and fcntl(). */
#define O_APPEND	02000	/* Set append mode */
#define O_NONBLOCK	04000	/* No delay */

/* File access modes for open() and fcntl(). */
#define O_RDONLY		0	/* Opens read only */
#define O_WRONLY		1	/* Opens write only */
#define O_RDWR			2	/* Opens read/write */

/* Mask for use with file access modes. */
#define O_ACCMODE		03	/* Mask for file access modes */

/* Struct used for locking. */
struct flock {
	short l_type;			/* Type: F_RDLCK, F_WRLCK, or F_UNLCK */
	short l_whence;			/* Flag for starting offset */
	off_t l_start;			/* Relative offset in bytes */
	off_t l_len;			/* Size; if 0, then until EOF */
	pid_t l_pid;			/* Process id of the locks' owner */
};

int fcntl(int fd, int cmd, ...);
int open(const char *path, int oflag, ...);

#endif
