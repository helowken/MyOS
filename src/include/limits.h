#ifndef _LIMITS_H
#define _LIMITS_H

#define CHAR_BIT	8	/* bits in a char */

#define INT_MIN		(-2147483647-1)
#define INT_MAX		2147483647
#define UINT_MAX	0xFFFFFFFF

#define LONG_MIN	(-2147483647L-1)
#define LONG_MAX	2147483647L
#define ULONG_MAX	0xFFFFFFFFL

#define NAME_MAX	DIR_SIZE	/* chars in a file name */

#define OPEN_MAX	20	/* # open files a process may have */

#endif
