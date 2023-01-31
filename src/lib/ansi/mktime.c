#include "time.h"
#include "limits.h"
#include "loc_time.h"

/* The code assumes that unsigned long can be converted to time_t.
 * A time_t should not be wider than unsigned long, since this would mean
 * that the check for overflow at the end could fail.
 */
time_t mktime(register struct tm *tm) {
	register long day, year;
	register int tmYear;
	int yday, month;
	register unsigned long seconds;
	int overflow;
	unsigned dst;

	tm->tm_min += tm->tm_sec / 60;
	tm->tm_sec %= 60;
	if (tm->tm_sec < 0) {
		tm->tm_sec += 60;
		tm->tm_min--;
	}
	tm->tm_hour += tm->tm_min / 60;
	tm->tm_min %= 60;
	if (tm->tm_min < 0) {
		tm->tm_min += 60;
		tm->tm_hour--;
	}
	day = tm->tm_hour / 24;
	tm->tm_hour %= 24;
	if (tm->tm_hour < 0) {
		tm->tm_hour += 24;
		day--;
	}
	tm->tm_year += tm->tm_mon / 12;
	tm->tm_mon %= 12;
	if (tm->tm_mon < 0) {
		tm->tm_mon += 12;
		tm->tm_year--;
	}
	day += tm->tm_mday - 1;
	while (day < 0) {
		if (--tm->tm_mon < 0) {
			tm->tm_year--;
			tm->tm_mon = 11;
		}
		day += _ytab[LEAP_YEAR(YEAR0 + tm->tm_year)][tm->tm_mon];
	}
	while (day >= _ytab[LEAP_YEAR(YEAR0 + tm->tm_year)][tm->tm_mon]) {
		day -= _ytab[LEAP_YEAR(YEAR0 + tm->tm_year)][tm->tm_mon];
		if (++tm->tm_mon == 12) {
			tm->tm_mon = 0;
			tm->tm_year++;
		}
	}
	tm->tm_mday = day + 1;
	_tzset();		/* Set timezone and dst info */
	year = EPOCH_YEAR;
	if (tm->tm_year < year - YEAR0)
	  return (time_t) -1;
	seconds = 0;
	day = 0;		/* Means days since day 0 now */
	overflow = 0;

	/* Assume that when day becomes negative, there will certainly
	 * be overflow on seconds.
	 * The check for overflow needs not to be done for leapyears
	 * divisible by 400.
	 * The code only works when year (1970) is not leapyear.
	 */
#if EPOCH_YEAR != 1970
#error EPOCH_YEAR != 1970
#endif
	tmYear = tm->tm_year + YEAR0;

	if (LONG_MAX / 365 < tmYear - year)
	  ++overflow;
	day = (tmYear - year) * 365;
	if (LONG_MAX - day < (tmYear - year) / 4 + 1)
	  ++overflow;
	day += (tmYear - year) / 4 + (tmYear % 4 && tmYear % 4 < year % 4);
	day -= (tmYear - year) / 100 + (tmYear % 100 && tmYear % 100 < year % 100);
	day += (tmYear - year) / 400 + (tmYear % 400 && tmYear % 400 < year % 400);

	yday = month = 0;
	while (month < tm->tm_mon) {
		yday += _ytab[LEAP_YEAR(tmYear)][month];
		++month;
	}
	yday += tm->tm_mday - 1;
	if (day + yday < 0)
	  ++overflow;
	day += yday;

	tm->tm_yday = yday;
	tm->tm_wday = (day + 4) % 7;	/* day 0 was thursday (4) */

	seconds = ((tm->tm_hour * 60L) + tm->tm_min) * 60L + tm->tm_sec;

	if ((TIME_MAX - seconds) / SECS_OF_DAY < day)
	  ++overflow;
	seconds += day * SECS_OF_DAY;

	/* Now adjust according to timezone and daylight saving time */
	if ((_timezone > 0 && TIME_MAX - _timezone < seconds) ||
			(_timezone < 0 && seconds < -_timezone))
	  ++overflow;
	seconds += _timezone;

	if (tm->tm_isdst < 0)
	  dst = _dstget(tm);
	else if (tm->tm_isdst)
	  dst = _dstOff;
	else
	  dst = 0;

	if (dst > seconds)
	  ++overflow;	/* dst is always non-negative */
	seconds -= dst;

	if (overflow)
	  return (time_t) -1;

	if ((time_t) seconds != seconds)
	  return (time_t) -1;
	return (time_t) seconds;
}



