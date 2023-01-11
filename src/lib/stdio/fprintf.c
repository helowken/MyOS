#include "stdio.h"
#include "stdarg.h"
#include "loc_incl.h"

int fprintf(FILE * stream, const char *format, ...) {
	va_list ap;
	int retVal;

	va_start(ap, format);
	retVal = _doPrint(format, ap, stream);
	va_end(ap);

	return retVal;
}
