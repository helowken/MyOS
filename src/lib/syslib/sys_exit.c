#include "syslib.h"

int sysExit(int pNum) {
/* A process has exited. PM tells the kernel. In addition this call can be
 * used by system processes to directly exit without passing through the
 * PM. This should be used with care to prevent inconsistent PM tables.
 */
	Message msg;

	msg.PR_PROC_NR = pNum;
	return taskCall(SYSTASK, SYS_EXIT, &msg);
}
