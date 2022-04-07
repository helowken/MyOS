#ifndef _SYS__TIME_H
#define	_SYS__TIME_H

struct timeval {
	time_t tv_sec;			/* Seconds */
	suseconds_t tv_usec;	/* Microseconds */
};

int gettimeofday(struct timeval *tv, void *tz);

int settimeofday(const struct timeval *tv, const void *tz);

#endif
