#include "lib.h"
#include "sys/resource.h"

int getpriority(int which, int who) {
	int v;
	Message msg;

	msg.m1_i1 = which;
	msg.m1_i2 = who;

	/* GETPRIORITY returns negative for error.
	 * Otherwise, it returns the priority plus the minimum
	 * priority, to distinginuish from error. We have to
	 * correct for this. (The user program has to check errno
	 * to see if something really went wrong.)
	 */

	if ((v = syscall(MM, GETPRIORITY, &msg)) < 0)
	  return v;

	return v + PRIO_MIN;
}

int setpriority(int which, int who, int prio) {
	Message msg;

	msg.m1_i1 = which;
	msg.m1_i2 = who;
	msg.m1_i3 = prio;

	return syscall(MM, SETPRIORITY, &msg);
}
