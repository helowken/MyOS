#include "syslib.h"

int sysGetInfo(int request, void *ptr, int len, void *ptr2, int len2) {
	Message msg;
	msg.I_REQUEST = request;
	msg.I_PROC_NR = SELF;	/* Always store values at caller */
	msg.I_VAL_PTR = ptr;
	msg.I_VAL_LEN = len;
	msg.I_VAL_PTR2 = ptr2;
	msg.I_VAL_LEN2 = len2;

	return taskCall(SYSTASK, SYS_GETINFO, &msg);
}
