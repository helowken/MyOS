#include <lib.h>
#include <sys/times.h>
#include <time.h>

clock_t times(struct tms *buf) {
	Message msg;

	if (syscall(MM, TIMES, &msg) < 0)
	  return (clock_t) -1;
	buf->tms_utime = msg.m4_l1;
	buf->tms_stime = msg.m4_l2;
	buf->tms_cutime = msg.m4_l3;
	buf->tms_cstime = msg.m4_l4;
	return 0;
}
