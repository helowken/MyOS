#include <lib.h>
#include <fcntl.h>
#include <stdarg.h>

int fcntl(int fd, int cmd, ...) {
	va_list argp;
	Message msg;

	va_start(argp, cmd);

	/* Set up for the sensible case where there is no variable parameter. This
	 * covers F_GETFD, F_GETFL and invalid commands.
	 */
	msg.m1_i3 = 0;
	msg.m1_p1 = NIL_PTR;

	/* Adjust for the stupid cases. */
	switch (cmd) {
		case F_DUPFD:
		case F_SETFD:
		case F_SETFL:
			msg.m1_i3 = va_arg(argp, int);
			break;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			msg.m1_p1 = (char *) va_arg(argp, struct flock *);
			break;
	}

	/* Clean up and make the system call. */
	va_end(argp);
	msg.m1_i1 = fd;
	msg.m1_i2 = cmd;
	return syscall(FS, FCNTL, &msg);
}

