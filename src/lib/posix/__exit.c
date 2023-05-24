#include <lib.h>
#include <unistd.h>

void _exit(int status) {
	Message msg;

	msg.m1_i1 = status;
	syscall(MM, EXIT, &msg);
}
