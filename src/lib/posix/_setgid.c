#include <lib.h>
#include <unistd.h>

int setgid(gid_t gid) {
	Message msg;

	msg.m1_i1 = (int) gid;
	return syscall(MM, SETGID, &msg);
}
