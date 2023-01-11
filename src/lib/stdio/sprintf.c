#include "stdio.h"
#include "stdarg.h"
#include "limits.h"
#include "loc_incl.h"

int sprintf(char *s, const char *format, ...) {
	va_list ap;
	int retVal;
	
	va_start(ap, format);
	retVal = vsnprintf(s, INT_MAX, format, ap);
	va_end(ap);

	return retVal;
}

int snprintf(char *s, size_t size, const char *format, ...) {
	va_list ap;
	int retVal;
	
	va_start(ap, format);
	retVal = vsnprintf(s, size, format, ap);
	va_end(ap);

	return retVal;
}
