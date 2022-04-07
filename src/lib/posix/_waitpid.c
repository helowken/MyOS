#include "lib.h"
#include "sys/wait.h"

pid_t waitpid(pid_t pid, int *status, int options) {
	Message msg;

	msg.m1_i1 = pid;
	msg.m1_i2 = options;
	if (syscall(MM, WAITPID, &msg) < 0)
	  return -1;
	if (status != 0)
	  *status = msg.m2_i1;
	return msg.m_type;
}
