#include <lib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

int open(const char *name, int flags, ...) {
	va_list argp;

	Message msg;

	va_start(argp, flags);
	if (flags & O_CREAT) {
		msg.m1_i1 = strlen(name) + 1;
		msg.m1_i2 = flags;
		msg.m1_i3 = va_arg(argp, _mnx_Mode_t);
		msg.m1_p1 = (char *) name;
	} else {
		_loadName(name, &msg);
		msg.m3_i2 = flags;
	}
	va_end(argp);
	
	return syscall(FS, OPEN, &msg);
}
