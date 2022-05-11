#include "syslib.h"
#include "stdarg.h"
#include "unistd.h"

int sysAbort(int how, ...) {
/* Something awful has happened. Abandon ship. */

	Message msg;
	va_list ap;
	
	// TODO 
}
