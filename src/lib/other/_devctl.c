#include "lib.h"
#include "unistd.h"

int devCtl(int ctlReq, int pNum, int device, int style) {
	Message msg;

	msg.m4_l1 = ctlReq;
	msg.m4_l2 = pNum;
	msg.m4_l3 = device;
	msg.m4_l4 = style;

	if (syscall(FS, DEVCTL, &msg) < 0)
	  return -1;
	return 0;
}
