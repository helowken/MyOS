#include <stdio.h>
#include <stdarg.h>
#include "loc_incl.h"

int vprintf(const char *format, va_list ap) {
	return _doPrint(format, ap, stdout);
}
