#ifndef _FCNTL_H
#define _FCNTL_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* Oflag values for open() */
#define O_CREAT		00100	/* Create file if it doesn't exist */
#define O_EXCL		00200	/* Exclusive use flag */
#define O_NOCTTY	00400	/* Do not assign a controlling terminal */
#define O_TRUNC		01000	/* Truncate flag */


/* File status flags for open() and fcntl(). */
#define O_APPEND	02000	/* Set append mode */
#define O_NONBLOCK	04000	/* No delay */

#endif
