#include "ctype.h"
#include "time.h"
#include "stdlib.h"
#include "string.h"
#include "loc_time.h"

#define RULE_LEN	120
#define TZ_LEN		10

/* Make sure that the strings do not end up in ROM.
 * These strings probably contain the wrong value, and we cannot obtain the
 * right value from the system. TZ is the only help.
 */
static char ntStr[TZ_LEN + 1] = "GMT";	/* String for normal time */
static char dstStr[TZ_LEN + 1] = "GDT";	/* String for daylight saving */

long _timezone = 0;
long _dstOff = 60 * 60;
int _daylight = 0;
char *_tzname[2] = {ntStr, dstStr};

#if defined(__USG) || defined(_POSIX_SOURCE)
char *tzname[2] = {ntStr, dstStr};

#if defined(__USG)
long timezone = 0;
int daylight = 0;
#endif
#endif

typedef struct {
	char ds_type;		/* Unknown, Julian, Zero-based or M */
	int ds_date[3];		/* Months, weeks, days */
	long ds_sec;		/* Usually 02:00:00 */
} DstType;

static DstType dstStart = {'U', {0, 0, 0}, 2 * 60 * 60};
static DstType dstEnd = {'U', {0, 0, 0}, 2 * 60 * 60};

const char *_days[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", 
	"Thursday", "Friday", "Saturday"
};

const char *_months[] = {
	"January", "February", "March", 
	"April", "May", "June",
	"July", "August", "September",
	"October", "November", "December"
};

const int _ytab[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static const char *parseZoneName(register char *buf, 
			register const char *p) {
/* Any characters except for a leading colon (:), digits, comma (,), 
 * minus (-), plus (+), and ASCII NUL (\0) * are allowed. 
 */
	register int n = 0;

	if (*p == ':')
	  return NULL;

	while (*p && ! isdigit(*p) && *p != ',' && 
				*p != '-' && *p != '+') {
		if (n < TZ_LEN)
		  *buf++ = *p;
		++p;
		++n;
	}
	/* std and dst are three or more letters. */
	if (n < 3)
	  return NULL;
	*buf = '\0';
	return p;
}

static int parseNum(const char **pp, int min, int max, char *buf) {
	register const char *p = *pp;
	register int n = 0;
	register const char *q = p;

	while (*p >= '0' && *p <= '9') {
		n = 10 * n + (*p - '0');
		if (buf != NULL)
		  *buf++ = *p;
		++p;
	}
	if (q == p)
	  return -1;		/* "Format error */
	if (n < min || n > max)
	  return -1;
	*pp = p;
	return n;
}

static const char *parseTime(register long *tm, const char *p, 
			register DstType *dst) {
/* Format: hh[:mm[:ss]] */
	register int n;
	char ds_type = (dst ? dst->ds_type : '\0');

	if (dst)
	  dst->ds_type = 'U';

	*tm = 0;
	if ((n = parseNum(&p, 0, 24, NULL)) == -1)
	  return NULL;
	*tm += n * 60 * 60;
	
	if (*p == ':') {
		if ((n = parseNum(&p, 0, 60, NULL)) == -1)
		  return NULL;
		*tm += n * 60;

		if (*p == ':') {
			if ((n = parseNum(&p, 0, 60, NULL)) == -1)
			  return NULL;
			*tm += n;
		}
	}
	if (dst) {
		dst->ds_type = ds_type;
		dst->ds_sec = *tm;
	}
	return p;
}

static const char *parseDate(register char *buf, const char *p, 
			DstType *dstInfo) {
/* The format of date may be one of the following:
 *  Jn      The Julian day n (1 <= n <= 365). Leap days aren't counted. 
 *		    That is, in all years - including leap years - February 28 
 *          is day 59 and March 1 is day 60. It's impossible to refer 
 *          explicitly to the occasional February 29.
 *
 *  n       The zero-based Julian day (0 <= n <= 365). Leap years are 
 *	        counted, and it is possible to refer to February 29.
 *
 *	Mm.n.d  The dth day (0 <= d <= 6) of week n of month m of the year 
 *	        (1 <= n <= 5, 1 <= m <= 12, where week 5 means "the last d 
 *	        day in month m", which may occur in the fourth or fifth 
 *	        week). Week 1 is the first week in which the dth day occurs. 
 *	        Day zero is Sunday.
 */
	int n = 0;
	int count = 0;
	const int bounds[3][2] = {	/* Mm.n.d */
		{1, 12},
		{1, 5},
		{0, 6}
	};
	char ds_type;

	if (*p != 'M') {
		if (*p == 'J') {
			*buf++ = *p++;
			ds_type = 'J';		/* Jn format */
		} else {
			ds_type = 'Z';		/* n format */
		}
		if ((n = parseNum(&p, ds_type == 'J', 365, buf)) == -1)
		  return NULL;
		dstInfo->ds_type = ds_type;
		dstInfo->ds_date[0] = n;
		return p;
	}
	
	ds_type = 'M';		/* Mm.n.d format */
	do {
		*buf++ = *p++;
		if ((n = parseNum(&p, bounds[count][0], 
						bounds[count][1], buf)) == -1)
		  return NULL;
		dstInfo->ds_date[count] = n;
		++count;
	} while (count < 3 && *p == '.');

	if (count != 3)
	  return NULL;

	*buf = '\0';
	dstInfo->ds_type = ds_type;
	return p;
}

static const char *parseRulePart(register char *buf, 
			register const char *p, DstType *dst) {
/* Format date/time.
 *
 * The default, if time is omitted, is 02:00:00.
 * (See the initialization of dstStart and dstEnd.)
 */
	long time;
	register const char *q;

	if ((p = parseDate(buf, p, dst)) == NULL)
	  return NULL;
	buf += strlen(buf);
	if (*p == '/') {
		q = ++p;
		if ((p = parseTime(&time, p, dst)) == NULL)
		  return NULL;
		while (p != q) {
			*buf++ = *q++;
		}
	}
	return p;
}

static const char *parseRule(register char *buf, register const char *p) {
/* Rule indicates when to change to and back from summer time. 
 * The rule has the form:
 *  date/time,date/time
 *
 * where the first date describes when the change from standard to 
 * summer time occurs, and the second date describes when the change 
 * back happens. 
 * Each time field describes when, in current local time, the change 
 * to the other time is made.
 */
	if ((p = parseRulePart(buf, p, &dstStart)) == NULL)
	  return NULL;
	if (*p != ',')
	  return NULL;
	++p;
	if ((p = parseRulePart(buf, p, &dstEnd)) == NULL)
	  return NULL;
	if (*p)
	  return NULL;
	return p;
}

/* The following routine parses timezone information in POSIX-format. For
 * the requirements, see IEEE Std 1003.1-1988 section 8.1.1.
 * The function returns as soon as it spots an error.
 *
 * TZ should be set as follows(spaces are for clarity only):
 * std offset dst offset, rule
 *
 * The expanded format is as follows:
 * stdoffset[dst[offset][,start[/time],end[/time]]]
 *
 * See https://users.pja.edu.pl/~jms/qnx/help/watcom/clibref/global_data.html for more.
 */
static void parseTZ(const char *p) {
	long tz, dst = 60 * 60, sign = 1;
	static char lastTZ[2 * RULE_LEN];
	static char buffer[RULE_LEN];

	if (!p)
	  return;

	if (*p == ':') {
		/* According to POSIX, this is implementation defined.
		 * Since it depends on the particular operating system, we
		 * can do nothing.
		 */
		return;
	}

	if (! strcmp(lastTZ, p))
	  return;	/* Nothing changed */

	*_tzname[0] = '\0';
	*_tzname[1] = '\0';
	dstStart.ds_type = 'U';
	dstStart.ds_sec = 2 * 60 * 60;
	dstEnd.ds_type = 'U';
	dstEnd.ds_sec = 2 * 60 * 60;

	if (strlen(p) > 2 * RULE_LEN)
	  return;
	strcpy(lastTZ, p);

	/* Parse std. Only std is required. */
	if (! (p = parseZoneName(buffer, p)))
	  return;

	/* Parse offset of std.
	 *
	 * If preceded by a "-", the time zone is east of the Prime 
	 * Meridian; otherwise it's west (which may be indicated by 
	 * an optional preceding "+").
	 */
	if (*p == '-') {
		sign = -1;
		++p;
	} else if (*p == '+') {
		++p;
	}

	/* Offset indicates the value one must add to the local time 
	 * to arrive at Coordinated Universal Time (UTC). 
	 *
	 * The offset has the form: hh[:mm[:ss]]
	 *
	 * The minutes (mm) and seconds (ss) are optional. The hour (hh) 
	 * is required, and may be a single digit.
	 *
	 * The offset following std is required.
	 */
	if (! (p = parseTime(&tz, p, NULL)))
	  return;

	tz *= sign;
	_timezone = tz;
	strncpy(_tzname[0], buffer, TZ_LEN);

	/* If dst is omitted, summer time doesn't apply in this locale. */
	if (! (_daylight = (*p != '\0')))
	  return;

	/* Parse dst */
	buffer[0] = '\0';
	if (! (p = parseZoneName(buffer, p)))
	  return;
	strncpy(_tzname[1], buffer, TZ_LEN);

	/* Parse offset of dst. 
	 * If no offset follows dst, summer time is assumed to be one hour 
	 * ahead of standard time.
	 */
	buffer[0] = '\0';
	if (*p && (*p != ',')) {
		if (! (p = parseTime(&dst, p, NULL)))
		  return;
	}
	_dstOff = dst;	/* Dst was initialized to 1 hour */
	
	if (*p) {
		if (*p != ',')
		  return;
		++p;
		if (strlen(p) > RULE_LEN)
		  return;
		/* Parse rule. */
		if (! (p = parseRule(buffer, p)))
		  return;
	}
}

static int dateOf(register DstType *dst, struct tm *tm) {
	int leap = LEAP_YEAR(YEAR0 + tm->tm_year);
	int firstDay, tmpDay;
	register int day, month;

	if (dst->ds_type != 'M') 
	  return dst->ds_date[0] - (dst->ds_type == 'J' && leap && dst->ds_date[0] < 58);

	day = 0;
	month = 1;
	while (month < dst->ds_date[0]) {
		day += _ytab[leap][month - 1];
		++month;
	}
	firstDay = (day + FIRST_DAY_OF(tm)) % 7;
	tmpDay = day;
	day += (dst->ds_date[2] - firstDay + 7) % 7 + 7 * (dst->ds_date[1] - 1);
	if (day >= tmpDay + _ytab[leap][month - 1])
	  day -= 7;
	return day;
}

static int lastSunday(register int day, register struct tm *tm) {
	int first = FIRST_SUNDAY(tm);

	if (day >= 58 && LEAP_YEAR(YEAR0 + tm->tm_year))
	  ++day;
	if (day < first)
	  return first;
	return day - (day - first) % 7;
}

/* The default dst transitions are those for Western Europe (except Great
 * Britain).
 */
unsigned _dstget(struct tm *tm) {
	int beginDst, endDst;
	register DstType *dsts = &dstStart, *dste = &dstEnd;
	int doDst = 0;

	if (_daylight == -1)
	  _tzset();

	tm->tm_isdst = _daylight;
	if (! _daylight)
	  return 0;

	if (dsts->ds_type != 'U') 
	  beginDst = dateOf(dsts, tm);
	else
	  beginDst = lastSunday(89, tm);	/* Last Sun before Apr */

	if (dste->ds_type != 'U')
	  endDst = dateOf(dste, tm);
	else
	  endDst = lastSunday(272, tm);		/* Last Sun in Sep */

	/* Assume beginDst != endDst (otherwise it would be no use) */
	if (beginDst < endDst) {	/* Northern hemisphere */
		if (tm->tm_yday > beginDst && tm->tm_yday < endDst)
		  doDst = 1;
	} else {	/* Southern hemisphere */
		if (tm->tm_yday > beginDst && tm->tm_yday < endDst)
		  doDst = 1;
	}	

	if (! doDst &&
			(tm->tm_yday == beginDst || tm->tm_yday == endDst)) {
		long dstTransSec;	/* Transition when day is this old */
		long curSec;

		if (tm->tm_yday == beginDst)
		  dstTransSec = dsts->ds_sec;
		else 
		  dstTransSec = dste->ds_sec;

		curSec = ((tm->tm_hour * 60) + tm->tm_min) * 60L + tm->tm_sec;

		if ((tm->tm_yday == beginDst && curSec >= dstTransSec) ||
				(tm->tm_yday == endDst && curSec < dstTransSec))
		  doDst = 1;
	}
	if (doDst)
	  return _dstOff;

	tm->tm_isdst = 0;
	return 0;
}

void _tzset() {
	parseTZ(getenv("TZ"));	
#if defined(__USG) || defined(_POSIX_SOURCE)
	tzname[0] = _tzname[0];
	tzname[1] = _tzname[1];
#if defined(__USG)
	timezone = _timezone;
	daylight = _daylight;
#endif
#endif
}



