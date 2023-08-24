#include <lib.h>
#include <minix/syslib.h>

int taskCall(int who, int sysCallNum, register Message *msg) {
	int status;

	msg->m_type = sysCallNum;
	status = sendRec(who, msg);
	if (status != 0)
	  return status;
	return msg->m_type;
}
