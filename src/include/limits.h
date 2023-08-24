#ifndef _LIMITS_H
#define _LIMITS_H

#define CHAR_BIT	8			/* Bits in a char */
#define CHAR_MIN	-128		/* Minimum value of a char */
#define CHAR_MAX	127			/* Maximum value of a char */
#define SCHAR_MIN	-128		/* Minimum value of a signed char */
#define SCHAR_MAX	127			/* Maximum value of a signed char */
#define UCHAR_MAX	255			/* Maximum value of an unsigned char */

#define SHRT_MIN	(-32767-1)	/* Minimum value of a short */
#define SHRT_MAX	32767		/* Maximum value of a short */
#define USHRT_MAX	0xFFFF		/* Maximum value of unsigned short */

#define INT_MIN		(-2147483647-1)
#define INT_MAX		2147483647
#define UINT_MAX	0xFFFFFFFF

#define LONG_MIN	(-2147483647L-1)
#define LONG_MAX	2147483647L
#define ULONG_MAX	0xFFFFFFFFL

#include "sys/dir.h"

/* Minimum sizes required by the POSIX */
#ifdef _POSIX_SOURCE
#define _POSIX_ARG_MAX		4096	/* exec() may have 4K worth of args */
#define _POSIX_CHILD_MAX	6		/* A process may have 6 children */
#define _POSIX_LINK_MAX		8		/* A file may have 8 links */
#define _POSIX_MAX_CAMON	255		/* Size of the canonical input queue */
#define _POSIX_MAX_INPUT	255		/* You can type 255 chars ahead */
#define _POSIX_NAME_MAX		DIRSIZ	/* A file name may have 60 chars */
#define _POSIX_NGROUPS_MAX	0		/* Supplementary group IDs are optional */
#define _POSIX_OPEN_MAX		16		/* A process may have 16 files open */
#define _POSIX_PATH_MAX		255		/* A pathname may contain 255 chars */
#define _POSIX_PIPE_BUF		512		/* Pipes writes of 512 bytes must be atomic */
#define _POSIX_STREAM_MAX	8		/* At least 8 FILEs can be open at once */
#define _POSIX_TZNAME_MAX	3		/* Time zone names can be at least 3 chars */
#define _POSIX_SSIZE_MAX	32767	/* read() must support 32767 byte reads */

/* Values actually implemented by MINIX */
#define _NO_LIMIT		100			/* Arbitrary number; limit not enforced */

#define NGROUPS_MAX		0			/* Supplemental group IDs not available */
#define ARG_MAX			16384		/* # bytes of args + environ for exec() */
#define CHILD_MAX		_NO_LIMIT	/* MINIX does lnot limit children */
#define OPEN_MAX		20			/* # open files a process may have */
#define LINK_MAX		SHRT_MAX	/* # links a file may have */
#define MAX_CANON		255			/* Size of the canonical input queue */
#define MAX_INPUT		255			/* Size of the type-ahead buffer */
#define NAME_MAX		DIRSIZ		/* # chars in a file name */
#define PATH_MAX		255			/* # chars in a path name */
#define PIPE_BUF		7168		/* # bytes in atomic write to a pipe */
#define STREAM_MAX		20			/* Must be the same as FOPEN_MAX in stdio.h */
#define TZNAME_MAX		3			/* Maximum bytes in a time zone name is 3 */
#define SSIZE_MAX		32767		/* Max defined byte count for read() */

#endif	/* _POSIX_SOURCE */

#endif	/* _LIMITS_H */
