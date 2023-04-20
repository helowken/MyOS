#include "lib.h"
#include "unistd.h"

int setuid(uid_t uid) {
	Message msg;

	msg.m1_i1 = uid;
	return syscall(MM, SETUID, &msg);
}
