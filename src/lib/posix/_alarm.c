#include <lib.h>
#include <unistd.h>

unsigned int alarm(unsigned int seconds) {
	Message msg;

	msg.m1_i1 = (int) seconds;
	return (unsigned) syscall(MM, ALARM, &msg);
}
