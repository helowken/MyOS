#include "time.h"
#include "loc_time.h"

/* The width can be -1 in both sPrint() as in uPrint(). This
 * indicates that as many characters as needed should be printed.
 */
static char *sPrint(char *s, size_t maxSize, const char *str, int width) {
	while (width > 0 || (width < 0 && *str)) {
		if (! maxSize)
		  break;
		*s++ = *str++;
		--maxSize;
		--width;
	}
	return s;
}

static char *uPrint(char *s, size_t maxSize, unsigned val, int width) {
	int c;

	c = val % 10;
	val = val / 10;
	if (--width > 0 || (width < 0 && val != 0))
	  s = uPrint(s, maxSize ? maxSize - 1 : 0, val, width);
	if (maxSize)
	  *s++ = c + '0';
	return s;
}

size_t strftime(char *s, size_t maxSize, const char *format, 
			const struct tm *tm) {
	size_t n;
	char *firsts, *olds;
	char c;
	char *fmt;

	if (! format)
	  return 0;

	_tzset();	/* For %Z conversion */
	firsts = s;
	while (maxSize && *format) {
		while (maxSize && *format && *format != '%') {
			*s++ = *format++;
			--maxSize;
		}
		if (! maxSize || ! *format)
		  break;
		++format;
		
		olds = s;
		switch ((c = *format++)) {
			case 'a':
				s = sPrint(s, maxSize, _days[tm->tm_wday], ABB_LEN);
				maxSize -= s - olds;
				break;
			case 'A':
				s = sPrint(s, maxSize, _days[tm->tm_wday], -1);
				maxSize -= s - olds;
				break;
			case 'b':
				s = sPrint(s, maxSize, _months[tm->tm_mon], ABB_LEN);
				maxSize -= s - olds;
				break;
			case 'B':
				s = sPrint(s, maxSize, _months[tm->tm_mon], -1);
				maxSize -= s - olds;
				break;
			case 'd':
				s = uPrint(s, maxSize, tm->tm_mday, 2);
				maxSize -= s - olds;
				break;
			case 'H':
				s = uPrint(s, maxSize, tm->tm_hour, 2);
				maxSize -= s - olds;
				break;
			case 'I':
				s = uPrint(s, maxSize, (tm->tm_hour + 11) % 12 + 1, 2);
				maxSize -= s - olds;
				break;
			case 'j':
				s = uPrint(s, maxSize, tm->tm_yday + 1, 3);
				maxSize -= s - olds;
				break;
			case 'm':
				s = uPrint(s, maxSize, tm->tm_mon + 1, 2);
				maxSize -= s - olds;
				break;
			case 'M':
				s = uPrint(s, maxSize, tm->tm_min, 2);
				maxSize -= s - olds;
				break;
			case 'p':
				s = sPrint(s, maxSize, (tm->tm_hour < 12) ? "AM" : "PM", 2);
				maxSize -= s - olds;
				break;
			case 'S':
				s = uPrint(s, maxSize, tm->tm_sec, 2);
				maxSize -= s - olds;
				break;
			case 'U':
				s = uPrint(s, maxSize, (tm->tm_yday + 7 - tm->tm_wday) / 7, 2);
				maxSize -= s - olds;
				break;
			case 'w':
				s = uPrint(s, maxSize, tm->tm_wday, 1);
				maxSize -= s - olds;
				break;
			case 'W':
				s = uPrint(s, maxSize, (tm->tm_yday + 7 - (tm->tm_wday + 6) % 7) / 7, 2);
				maxSize -= s - olds;
				break;
			case 'c':
			case 'x':
			case 'X':
				if (c == 'c')
				  fmt = "%a %b %d %H:%M:%S %Y";
				else if (c == 'x')
				  fmt = "%a %b %d %Y";
				else
				  fmt = "%H:%M:%S";
				n = strftime(s, maxSize, fmt, tm);
				if (n)
				  maxSize -= n;
				else
				  maxSize = 0;
				s += n;
				break;
			case 'y':
				s = uPrint(s, maxSize, tm->tm_year % 100, 2);
				maxSize -= s - olds;
				break;
			case 'Y':
				s = uPrint(s, maxSize, tm->tm_year + YEAR0, -1);
				maxSize -= s - olds;
				break;
			case 'Z':
				s = sPrint(s, maxSize, _tzname[(tm->tm_isdst > 0)], -1);
				maxSize -= s - olds;
				break;
			case '%':
				*s++ = '%';
				--maxSize;
				break;
			default:
				/* A conversion error. Leave the loop. */
				while (*format) {
					++format;
				}
				break;
		}
	}
	if (maxSize) {
		*s = '\0';
		return s - firsts;
	}
	return 0;	/* The buffer is full. */

}
