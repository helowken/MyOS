#include <time.h>
#include <limits.h>
#include "loc_time.h"

struct tm *gmtime(const time_t *tp) {
	static struct tm brTime;
	register struct tm *tm = &brTime;
	time_t time = *tp;
	register unsigned long dayClock, dayNum;
	int year = EPOCH_YEAR;

	dayClock = (unsigned long) time % SECS_OF_DAY;
	dayNum = (unsigned long) time / SECS_OF_DAY;

	tm->tm_sec = dayClock % 60;
	tm->tm_min = (dayClock % 3600) / 60;
	tm->tm_hour = dayClock / 3600;
	tm->tm_wday = (dayNum + 4) % 7;		/* Day 0 was a thursday */
	while (dayNum >= YEAR_SIZE(year)) {
		dayNum -= YEAR_SIZE(year);
		++year;
	}
	tm->tm_year = year - YEAR0;
	tm->tm_yday = dayNum;
	tm->tm_mon = 0;
	while (dayNum >= _ytab[LEAP_YEAR(year)][tm->tm_mon]) {
		dayNum -= _ytab[LEAP_YEAR(year)][tm->tm_mon];
		++tm->tm_mon;
	}
	tm->tm_mday = dayNum + 1;
	tm->tm_isdst = 0;
	return tm;
}
