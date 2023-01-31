#define YEAR0			1900
#define EPOCH_YEAR		1970	/* The first year */
#define SECS_OF_DAY		(24L * 60L * 60L)
#define LEAP_YEAR(year)		(!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEAR_SIZE(year)		(LEAP_YEAR(year) ? 366 : 365)
#define FIRST_SUNDAY(tm)	(((tm)->tm_yday - (tm)->tm_wday + 420) % 7)
#define FIRST_DAY_OF(tm)	(((tm)->tm_wday - (tm)->tm_yday + 420) % 7)
#define TIME_MAX		ULONG_MAX
#define ABB_LEN			3

extern const int _ytab[2][12];
extern const char *_days[];
extern const char *_months[];

void _tzset(void);
unsigned _dstget(struct tm *tm);

extern long _timezone;
extern long _dstOff;
extern int _daylight;
extern char *_tzname[2];

