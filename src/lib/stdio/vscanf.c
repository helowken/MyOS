#include <stdio.h>
#include <stdarg.h>
#include "loc_incl.h"

int vscanf(const char *format, va_list ap) {
	return _doScan(stdin, format, ap);
}
