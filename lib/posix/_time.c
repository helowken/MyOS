#include "lib.h"
#include "time.h"

time_t time(time_t *tp) {
	Message msg;

	if (syscall(MM, TIME, &msg) < 0)
	  return (time_t) -1;

	if (tp != (time_t) 0)
	  *tp = msg.m2_l1;

	return msg.m2_l1;
}
