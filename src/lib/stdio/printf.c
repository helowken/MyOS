#include "stdio.h"
#include "stdarg.h"
#include "loc_incl.h"

int printf(const char *format, ...) {
	va_list ap;
	int retVal;

	va_start(ap, format);
	retVal = _doPrint(format, ap, stdout);
	va_end(ap);

	return retVal;
}
