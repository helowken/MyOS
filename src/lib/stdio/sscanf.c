#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "loc_incl.h"

int sscanf(const char *s, const char *format, ...) {
	va_list ap;
	int retVal;
	FILE tmpStream;

	va_start(ap, format);
	
	tmpStream._fd = -1;
	tmpStream._flags = _IOREAD | _IONBF | _IOREADING;
	tmpStream._buf = (unsigned char *) s;
	tmpStream._ptr = (unsigned char *) s;
	tmpStream._count = strlen(s);

	retVal = _doScan(&tmpStream, format, ap);

	va_end(ap);

	return retVal;
}
