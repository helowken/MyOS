#include <sys/types.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define MIN		60L			/* Seconds in a minute */
#define HOUR	(60 * MIN)	/* Seconds in an hour */
#define DAY		(24 * HOUR)	/* Seconds in a day */
#define YEAR	(365 * DAY)	/* Seconds in a (non-leap) year */

static int qflag, uflag, sflag, Sflag;

/* Default output file descriptor. */
static int outFd = 1;

/* Replacement for stdio putchar(). */
static void putchar(int c) {
	static char buf[1024];
	static char *bp = buf;

	if (c != 0)
	  *bp++ = c;
	if (c == 0 || c == '\n' || bp == buf + sizeof(buf)) {
		write(outFd, buf, bp - buf);
		bp = buf;
	}
}

/* Internal function that prints a fixed-size string. Replaces stdio in our
 * specific case.
 */
static void printString(const char *s, int len) {
	while (*s) {
		if (len--) 
		  putchar(*s++);
		else
		  break;
	}
}

static void printLongDecimal(unsigned long d, int digits) {
	--digits;
	if (d > 9 || digits > 0)
	  printLongDecimal(d / 10, digits);
	putchar('0' + (d % 10));
}

static void printDecimal(int d, int digits) {
	printLongDecimal((unsigned long) d, digits);
}

/* (Extended) Posix prototype of date. */
static void usage() {
	outFd = 2;
	printString("Usage: date [-qsuS] [-r seconds] [[MMDDYY]hhmm[ss]] [+format]\n", -1);
	exit(1);
}

/* Format the date. using the given locale string. A special case is the
 * TZ which might be a sign followed by four digits (New format time zone).
 */
static void formatDate(char *format, time_t t, struct tm *tm) {
	int i;
	char *s;
	static const char *wday[] = {
		"Sunday", "Monday", "Tuesday", "Wednesday", 
		"Thursday", "Friday", "Saturday"
	};

	static const char *month[] = {
		"January", "February", "March", 
		"April", "May", "June",
		"July", "August", "September",
		"October", "November", "December"
	};

	while (*format) {
		if (*format == '%') {
			switch (*++format) {
				case 'A':
					printString(wday[tm->tm_wday], -1);
					break;
				case 'B':
					printString(month[tm->tm_mon], -1);
					break;
				case 'D':
					printDecimal(tm->tm_mon + 1, 2);
					putchar('/');
					printDecimal(tm->tm_mday, 2);
					putchar('/');
					/* Fall through */
				case 'y':
					printDecimal(tm->tm_year % 100, 2);
					break;
				case 'H':
					printDecimal(tm->tm_hour, 2);
					break;
				case 'I':
					i = tm->tm_hour % 12;
					printDecimal(i ? i : 12, 2);
					break;
				case 'M':
					printDecimal(tm->tm_min, 2);
					break;
				case 'X':
				case 'T':
					printDecimal(tm->tm_hour, 2);
					putchar(':');
					printDecimal(tm->tm_min, 2);
					putchar(':');
					/* Fall through */
				case 'S':
					printDecimal(tm->tm_sec, 2);
					break;
				case 'U':	
					/* Decimal week number, Sunday being first day of week. */
					printDecimal((tm->tm_yday - tm->tm_wday + 13) / 7, 2);
					break;
				case 'W':
					/* Decimal week number, Monday being first day of week. */
					if (--tm->tm_wday < 0)
					  tm->tm_wday = 6;
					printDecimal((tm->tm_yday - tm->tm_wday + 13) / 7, 2);
					if (++tm->tm_wday > 6)
					  tm->tm_wday = 0;
					break;
				case 'Y':
					printDecimal(tm->tm_year + 1900, 4);
					break;
				case 'Z':	/* Time Zone (if any) */
					if (uflag)
					  s = "GMT";
					else
					  s = tm->tm_isdst == 1 ? tzname[1] : tzname[0];
					printString(s, strlen(s));
					break;
				case 'a':
					printString(wday[tm->tm_wday], 3);
					break;
				case 'b':
				case 'h':
					printString(month[tm->tm_mon], 3);
					break;
				case 'c':
					if (! (s = getenv("LC_TIME")))
					  s = "%a %b %e %T %Z %Y";
					formatDate(s, t, tm);
					break;
				case 'd':
					printDecimal(tm->tm_mday, 2);
					break;
				case 'e':
					if (tm->tm_mday < 10)
					  putchar(' ');
					printDecimal(tm->tm_mon, 1);
					break;
				case 'j':
					printDecimal(tm->tm_yday + 1, 3);
					break;
				case 'm':
					printDecimal(tm->tm_mon + 1, 2);
					break;
				case 'n':
					putchar('\n');
					break;
				case 'p':
					if (tm->tm_hour < 12)
					  putchar('A');
					else
					  putchar('P');
					putchar('M');
					break;
				case 'r':
					formatDate("%I:%M:%S %p", t, tm);
					break;
				case 's':
					printDecimal((unsigned long) t, 0);
					break;
				case 't':
					putchar('\t');
					break;
				case 'w':
					putchar('0' + tm->tm_wday);
					break;
				case 'x':
					formatDate("%B %e %Y", t, tm);
					break;
				case '%':
					putchar('%');
					break;
				case '\0':
					--format;
			}
			++format;
		} else
		  putchar(*format++);
	}
}

/* Convert a local date string into GMT time in seconds.
 * Example:
 *  date 0221921610		# Set date to Feb 21, 1992, at 4:10 p.m.
 */
static time_t makeTime(char *t) {
	struct tm tm;	/* User specified time */
	time_t now;		/* Current time */
	int leap;		/* Current year is leap year */
	int i;			/* General index */
	int idx;		/* Number of fields */
	int fields[6];	/* Time fields */
	static int daysPerMonth[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};

	/* Get current time just in case */
	now = time(NULL);
	tm = *localtime(&now);
	tm.tm_sec = 0;
	tm.tm_mon++;
	tm.tm_year %= 100;

	for (idx = 0; idx < 6; ++idx) {
		if (*t == 0)
		  break;
		fields[idx] = 0;
		for (i = 0; i < 2; ++i, ++t) {
			if (*t < '0' || *t >'9')
			  usage();
			fields[idx] = fields[idx] * 10 + (*t - '0');
		}
	}

	switch (idx) {
		case 2:		/* hhmm */
			/* Fall through */
		case 3:		/* hhmmss */
			tm.tm_hour = fields[0];
			tm.tm_min = fields[1];
			if (idx == 3) 
			  tm.tm_sec = fields[2];
			break;
		case 5:		/* MMDDYYhhmm */
			/* Fall through */
		case 6:		/* MMDDYYhhmmss */
			tm.tm_mon = fields[0];
			tm.tm_mday = fields[1];
			tm.tm_year = fields[2];
			tm.tm_hour = fields[3];
			tm.tm_min = fields[4];
			if (idx == 6) 
			  tm.tm_sec = fields[5];
			break;
		default:
			usage();
	}

	/* Convert the time into seconds since 1 January 1970 */
	if (tm.tm_year < 70)
	  tm.tm_year += 100;
	leap = (tm.tm_year % 4 == 0 && tm.tm_year % 400 != 0);
	if (tm.tm_mon < 1 || tm.tm_mon > 12 ||
		tm.tm_mday < 1 || tm.tm_mday > daysPerMonth[leap][tm.tm_mon - 1] ||
		tm.tm_hour > 23 || tm.tm_min > 59) {
		outFd = 2;
		printString("Illegal date format\n", -1);
		exit(1);
	}

	/* Convert the time into Minix time - zone independent code */
	{
		time_t utcTime;		/* Guess at unix time */
		time_t nextBit;		/* Next bit to try */
		int rv;				/* Result of try */
		struct tm *tmp;		/* Local time conversion */

#define COMPARE(a, b)	((a) != (b)) ? ((a) - (b)) : 

		utcTime = 1;
		do {
			nextBit = utcTime;
			utcTime = nextBit << 1;
		} while (utcTime >= 1);

		for (utcTime = 0; ; nextBit >>= 1) {
			utcTime |= nextBit;
			tmp = localtime(&utcTime);
			if (tmp == 0)
			  continue;

			rv = COMPARE(tmp->tm_year, tm.tm_year)
				 COMPARE(tmp->tm_mon + 1, tm.tm_mon)
				 COMPARE(tmp->tm_mday, tm.tm_mday)
				 COMPARE(tmp->tm_hour, tm.tm_hour)
				 COMPARE(tmp->tm_min, tm.tm_min)
				 COMPARE(tmp->tm_sec, tm.tm_sec)
				 0;
			
			if (rv > 0)
			  utcTime &= ~nextBit;
			else if (rv == 0)
			  break;

			if (nextBit == 0) {
				uflag = 1;
				outFd = 2;
				printString("Inexact conversion to UTC from ", -1);
				formatDate("%c\n", utcTime, localtime(&utcTime));
				exit(1);
			}
		}
		return utcTime;
	}
}

/* Correct the time to the reckoning of Eternal September. */
struct tm *september(time_t *tp) {
	time_t t;
	int days;
	struct tm *tm;

	tm = localtime(tp);
	t = *tp - (tm->tm_hour - 12) * HOUR;	/* No zone troubles around noon. */
	days = 0;

	while (tm->tm_year > 93 || (tm->tm_year == 93 && tm->tm_mon >= 8)) {
		/* Step back a year or a mont. */
		days += tm->tm_year > 93 ? tm->tm_yday + 1 : tm->tm_mday;
		t = *tp - days * DAY;
		tm = localtime(&t);
	}

	if (days > 0) {
		tm = localtime(tp);
		tm->tm_mday = days;
		tm->tm_year = 93;
		tm->tm_mon = 0;
	}
	return tm;
}

/* Main module. Handles P1003.2 date and system administrator's date. The
 * date entered should be given GMT, regardless of the system's TZ!
 */
int main(int argc, char **argv) {
	time_t t;
	struct tm *tm;
	char *format;
	char timeBuf[40];
	int n, i;

	time(&t);
	
	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1, *end;

		if (opt[0] == '-' && opt[1] == 0)
		  break;

		while (*opt != 0) {
			switch (*opt++) {
				case 'q':	/* Read the date from stdin. */
					qflag = 1;
					break;
				case 's':	/* Set the time. */
					sflag = 1;
					break;
				case 'S':	/* Date within Eternal September. */
					Sflag = 1;
					break;
				case 'u':	/* Print the date as GMT. */
					uflag = 1;
					break;
				case 'r':	/* Use the number of seconds instead of current time. */
					if (*opt == 0) {
						if (i == argc)
						  usage();
						opt = argv[i++];
					}
					t = strtoul(opt, &end, 10);
					if (*end != 0)
					  usage();
					opt = "";
					break;
				default:
					usage();
			}
		}
	}

	if (!qflag && i < argc && ('0' <= argv[i][0] && argv[i][0] <= '9')) {
		t = makeTime(argv[i++]);
		sflag = 1;
	}

	format = "%c";
	if (i < argc && argv[i][0] == '+')
	  format = argv[i++] + 1;

	if (i != argc)
	  usage();

	if (qflag) {
		printString("\nPlease enter date: MMDDYYhhmmss. Then hit the RETURN key.\n", -1);
		n = read(STDIN_FILENO, timeBuf, sizeof(timeBuf));
		if (n > 0 && timeBuf[n - 1] == '\n')
		  --n;
		if (n >= 0)
		  timeBuf[n] = '\0';
		t = makeTime(timeBuf);
		sflag = 1;
	}

	if (sflag && stime(&t) != 0) {
		outFd = 2;
		printString("No permission to set time\n", -1);
		return 1;
	}

	tm = Sflag ? september(&t) : (uflag ? gmtime(&t) : localtime(&t));

	formatDate(format, t, tm);
	putchar('\n');
	return 0;
}
