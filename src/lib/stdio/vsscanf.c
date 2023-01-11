#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "loc_incl.h"

int vsscanf(const char *s, const char *format, va_list ap) {
	FILE tmpStream;

	tmpStream._fd = -1;
	tmpStream._flags = _IOREAD | _IONBF | _IOREADING;
	tmpStream._buf = (unsigned char *) s;
	tmpStream._ptr = (unsigned char *) s;
	tmpStream._count = strlen(s);

	return _doScan(&tmpStream, format, ap);
}
