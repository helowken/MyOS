#include <lib.h>
#include <sys/time.h>
#include <time.h>

int gettimeofday(struct timeval *tv, void *tz) {
	Message msg;

	if (syscall(MM, GETTIMEOFDAY, &msg) < 0)
	  return -1;

	tv->tv_sec = msg.m2_l1;
	tv->tv_usec = msg.m2_l2;

	return 0;
}
