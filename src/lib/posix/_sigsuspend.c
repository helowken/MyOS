#include <lib.h>
#include <signal.h>

int sigsuspend(const sigset_t *set) {
	Message msg;

	msg.m2_l1 = (long) *set;
	return syscall(MM, SIGSUSPEND, &msg);
}
