#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include "loc_incl.h"

int vsnprintf(char *s, size_t size, const char *format, va_list ap) {
	int retVal;
	FILE tmpStream;

	tmpStream._fd = -1;
	tmpStream._flags = _IOWRITE | _IONBF | _IOWRITING;
	tmpStream._buf = (unsigned char *) s;
	tmpStream._ptr = (unsigned char *) s;
	tmpStream._count = size - 1;
	
	retVal = _doPrint(format, ap, &tmpStream);
	tmpStream._count = 1;
	putc('\0', &tmpStream);

	return retVal;
}

int vsprintf(char *s, const char *format, va_list ap) {
	return vsnprintf(s, INT_MAX, format, ap);
}
