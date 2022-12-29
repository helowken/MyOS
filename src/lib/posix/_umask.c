#include "lib.h"
#include "sys/stat.h"

mode_t umask(mode_t mask) {
	Message msg;

	msg.m1_i1 = mask;
	return (mode_t) syscall(FS, UMASK, &msg);
}

