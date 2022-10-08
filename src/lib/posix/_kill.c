#include "lib.h"
#include "signal.h"

int kill(int proc, int sig) {
	Message msg;

	msg.m1_i1 = proc;
	msg.m1_i2 = sig;
	return syscall(MM, KILL, &msg);
}
