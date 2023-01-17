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

struct tm {
	int tm_sec;				/* Seconds after the minute [0, 59] */
	int tm_min;				/* Minutes after the hour [0, 59] */
	int tm_hour;			/* Hours since midnight [0, 23] */
	int tm_mday;			/* Day of the month [1, 31] */
	int tm_mon;				/* Months since January [0, 11] */
	int tm_year;			/* Years since 1900 */
	int tm_wday;			/* Days since Sunday [0, 6] */
	int tm_yday;			/* Days since January 1 [0, 365] */
	int tm_isdst;			/* Daylight Saving Time flag */
};

time_t time(time_t *tp);	

#ifdef _MINIX
int stime(const time_t *t);
#endif

#endif
