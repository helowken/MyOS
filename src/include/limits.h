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

#define NAME_MAX	DIRSIZ		/* # chars in a file name */
#define PATH_MAX	255			/* # chars in a path name */

#define OPEN_MAX	20			/* # open files a process may have */

#define ARG_MAX		16384		/* # bytes of args + environ for exec() */

#endif
