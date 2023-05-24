#include "../system.h"
#include <unistd.h>

int doAbort(Message *msg) {
/* Handle sysAbort. MINIX is unable to continue. This can originate in the
 * PM (normal abort or panic) or TTY (after CTRL_ALT_DEL).
 */
	int how = msg->ABORT_HOW;
	int pNum;
	int length;
	phys_bytes srcPhys;

	/* See if the monitor is to run the specified instructions. */
	if (how == RBT_MONITOR) {
		pNum = msg->ABORT_MON_PROC;	
		if (! isOkProcNum(pNum))
		  return EINVAL;
		length = msg->ABORT_MON_LEN + 1;
		if (length > kernelInfo.paramsSize)
		  return E2BIG;
		srcPhys = nUMapLocal(pNum, (vir_bytes) msg->ABORT_MON_ADDR, length);
		if (! srcPhys)
		  return EFAULT;

		/* Parameters seem ok, copy them and prepare shutting down. */
		physCopy(srcPhys, kernelInfo.paramsBase, (phys_bytes) length);
	}

	/* Now prepare to shutdown MINIX. */
	prepareShutdown(how);
	return OK;
}
