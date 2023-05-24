#include <lib.h>
#include <sys/wait.h>

pid_t wait(int *status) {
	Message msg;

	if (syscall(MM, WAIT, &msg) < 0)
	  return -1;
	if (status != 0)
	  *status = msg.m2_i1;
	return msg.m_type;
}
