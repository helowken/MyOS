#include "string.h"
#include "time.h"
#include "loc_time.h"

#define DATE_STR	"??? ??? ?? ??:??:?? ????\n"

static char *twoDigits(register char *pb, int i, int noSpace) {
	*pb = (i / 10) % 10 + '0';
	if (! noSpace && *pb == '0')
	  *pb = ' ';
	++pb;
	*pb++ = (i % 10) + '0';
	return ++pb;
}

static char *fourDigits(register char *pb, int i) {
	i %= 10000;
	*pb++ = (i / 1000) + '0';
	i %= 1000;
	*pb++ = (i / 100) + '0';
	i %= 100;
	*pb++ = (i / 10) + '0';
	*pb++ = (i % 10) + '0';
	return ++pb;
}

char *asctime(const struct tm *tm) {
	static char buf[26];
	register char *pb = buf;
	register const char *ps;
	register int n;

	strcpy(pb, DATE_STR);
	ps = _days[tm->tm_wday];
	n = ABB_LEN;
	while (--n >= 0) {
		*pb++ = *ps++;
	}
	++pb;

	ps = _months[tm->tm_mon];
	n = ABB_LEN;
	while (--n >= 0) {
		*pb++ = *ps++;
	}
	++pb;

	pb = twoDigits(pb, tm->tm_mday, 0);
	pb = twoDigits(pb, tm->tm_hour, 1);
	pb = twoDigits(pb, tm->tm_min, 1);
	pb = twoDigits(pb, tm->tm_sec, 1);

	fourDigits(pb, tm->tm_year + 1900);
	return buf;
}
