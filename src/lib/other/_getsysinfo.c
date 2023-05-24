#include <lib.h>
#include <unistd.h>

int getSysInfo(int who, int what, void *where) {
	Message msg;

	msg.m1_i1 = what;
	msg.m1_p1 = where;
	if (syscall(who, GETSYSINFO, &msg) < 0)
	  return -1;
	return 0;
}
