#ifndef _UTIME_H
#define _UTIME_H

#ifndef	_TYPES_H
#include "sys/types.h"
#endif

struct utimebuf {
	time_t actime;		/* Access time */
	time_t modtime;		/* Modification time */
};

int utime(const char *path, const struct utimebuf *times);

#endif
