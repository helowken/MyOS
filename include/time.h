#ifndef _TIME_H
#define _TIME_H

#ifndef	_TIME_T
#define _TIME_T
typedef	long time_t;		/* Time in sec since 1 Jan 1970 0000 GMT */
#endif

#ifndef _CLOCK_T
#define	_CLOCK_T
typedef long clock_t;		/* Time in ticks since process started */
#endif

int stime(const time_t *t);
time_t time(time_t *tp);	

#endif
