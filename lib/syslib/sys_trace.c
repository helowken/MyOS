#include "syslib.h"

int sysTrace(int req, int pNum, long addr, long *data) {
	Message msg;
	int r;

	msg.CTL_PROC_NR = pNum;
	msg.CTL_REQUEST = req;
	msg.CTL_ADDRESS = addr;
	if (data)
	  msg.CTL_DATA = *data;
	r = taskCall(SYSTASK, SYS_TRACE, &msg);
	if (data)
	  *data = msg.CTL_DATA;
	return r;
}
