#include <lib.h>
#include <unistd.h>
#include <stdarg.h>

int reboot(int how, ...) {
	Message msg;
	va_list ap;

	va_start(ap, how);
	if ((msg.m1_i1 = how) == RBT_MONITOR) {
		msg.m1_p1 = va_arg(ap, char *);
		msg.m1_i2 = va_arg(ap, size_t);
	}
	va_end(ap);

	return syscall(MM, REBOOT, &msg);
}
