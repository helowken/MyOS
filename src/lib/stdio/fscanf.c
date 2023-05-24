#include <stdio.h>
#include <stdarg.h>
#include "loc_incl.h"

int fscanf(FILE *stream, const char *format, ...) {
	va_list ap;
	int retVal;

	va_start(ap, format);
	retVal = _doScan(stream, format, ap);
	va_end(ap);

	return retVal;
}

