#include <lib.h>

int syscall(int who, int callNum, register Message *msg) {
	int status;

	msg->m_type = callNum;
	status = sendRec(who, msg);
	if (status != 0) {
		msg->m_type = status;
	} 
	if (msg->m_type < 0) {
		errno = -msg->m_type;
		return -1;
	}
	return msg->m_type;
}
