#include "lib.h"
#include "stddef.h"
#include "signal.h"

int sigprocmask(int how, const sigset_t *set, sigset_t *oldSet) {
	Message msg;

	if (set == (sigset_t *) NULL) {
		msg.m2_i1 = SIG_INQUIRE;
		msg.m2_l1 = 0;
	} else {
		msg.m2_i1 = how;
		msg.m2_l1 = (long) *set;
	}
	if (syscall(MM, SIGPROCMASK, &msg) < 0)
	  return -1;
	if (oldSet != (sigset_t *) NULL)
	  *oldSet = (sigset_t) (msg.m2_l1);
	return msg.m_type;
}
