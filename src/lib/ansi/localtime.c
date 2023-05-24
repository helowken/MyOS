#include <time.h>
#include "loc_time.h"

/* We must be careful, since an int can't represent all the seconds in a day.
 * Hence the adjustment of minutes when adding timezone and dst information.
 * This assumes that both must be expressable in multiples of a minute.
 * Furthermore, it is assumed that both fit into an integer when expressed as
 * minutes (this is about 22 days, so this should not cause any problems).
 */
struct tm *localtime(const time_t *tp) {
	struct tm *tm;	
	unsigned dst;

	_tzset();
	tm = gmtime(tp);	/* tm->tm_isdst == 0 */
	tm->tm_min -= _timezone / 60;
	tm->tm_sec -= _timezone % 60;
	mktime(tm);

	dst = _dstget(tm);
	if (dst) {
		tm->tm_min += dst / 60;
		tm->tm_sec += dst % 60;
		mktime(tm);
	}
	return tm;
}
