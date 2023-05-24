#include <lib.h>
#include <sys/time.h>
#include <sys/select.h>

int select(int nfds, fd_set *readfds, fd_set *writefds, 
			fd_set *errorfds, struct timeval *timeout) {
	Message msg;

	msg.SEL_NFDS = nfds;
	msg.SEL_READFDS = (char *) readfds;
	msg.SEL_WRITEFDS = (char *) writefds;
	msg.SEL_ERRORFDS = (char *) errorfds;
	msg.SEL_TIMEOUT = (char *) timeout;

	return syscall(FS, SELECT, &msg);
}
