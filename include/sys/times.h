#ifndef _TIMES_H
#define _TIMES_H

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long clock_t		/* Unit for system accounting */
#endif

struct tms {
	clock_t tms_utime;		/* User time */
	clock_t tms_stime;		/* System time */
	clock_t tms_cutime;		/* User time of children */
	clock_t tms_cstime;		/* System time of children */
};

clock_t times(struct tms *buf);

#endif
