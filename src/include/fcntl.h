#ifndef _FCNTL_H
#define _FCNTL_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* File status flags for open() and fcntl(). */
#define O_APPEND		02000	/* Set append mode */
#define O_NONBLOCK		04000	/* No delay */

#endif
