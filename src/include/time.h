#ifndef _TIME_H
#define _TIME_H

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

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
	int tm_isdst;			/* Daylight Saving Time flag:
							 *  positive - in effect
						     *	zero     - not set
							 *  negative - information is not available
							 */
};

extern char *tzname[];

clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t mktime(struct tm *tm);
time_t time(time_t *tp);	
char *asctime(const struct tm *tm);
char *ctime(const time_t *tp);
struct tm *gmtime(const time_t *tp);
struct tm *localtime(const time_t *tp);
size_t strftime(char *s, size_t max, const char *fmt, 
			const struct tm *tm);

extern long timezone;

#ifdef _POSIX_SOURCE
void tzset(void);
#endif

#ifdef _MINIX
int stime(const time_t *t);
#endif

#endif
