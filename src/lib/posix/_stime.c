#include <lib.h>
#include <time.h>

int stime(const time_t *t) {
	Message msg;

	msg.m2_l1 = *t;
	return syscall(MM, STIME, &msg);
}
