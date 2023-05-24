#include "syslib.h"
#include <stdarg.h>
#include <unistd.h>

int sysAbort(int how, ...) {
/* Something awful has happened. Abandon ship. */

	Message msg;
	va_list ap;

	va_start(ap, how);
	if ((msg.ABORT_HOW = how) == RBT_MONITOR) {
		msg.ABORT_MON_PROC = va_arg(ap, int);
		msg.ABORT_MON_ADDR = va_arg(ap, char *);
		msg.ABORT_MON_LEN = va_arg(ap, size_t);
	}
	va_end(ap);

	return taskCall(SYSTASK, SYS_ABORT, &msg);
}
