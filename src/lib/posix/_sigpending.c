#include "lib.h"
#include "signal.h"

int sigpending(sigset_t *set) {
	Message msg;

	if (syscall(MM, SIGPENDING, &msg) < 0)
	  return -1;
	*set = (sigset_t) msg.m2_l1;
	return msg.m_type;
}
