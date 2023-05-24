#include <stdio.h>
#include <stdarg.h>
#include "loc_incl.h"

int scanf(const char *format, ...) {
	va_list ap;
	int retVal;

	va_start(ap, format);
	retVal = _doScan(stdin, format, ap);
	va_end(ap);

	return retVal;
}
