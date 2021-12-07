#include "../system.h"

int doTimes(register Message *msg) {
/* Handle sys_times(). Retrieve the accounting information. */
	register Proc *rp;
	int pNum;

	/* Insert the times needed by the SYS_TIMES kernel call in the message.
	 * The clock's interrupt handler may run to update the user or system time
	 * while in this code, but that cannot do any harm.
	 */
	pNum = (msg->T_PROC_NR == SELF) ? msg->m_source : msg->T_PROC_NR;
	if (isOkProcNum(pNum)) {
		rp = procAddr(pNum);
		msg->T_USER_TIME = rp->p_user_time;
		msg->T_SYSTEM_TIME = rp->p_sys_time;
	}
	msg->T_BOOT_TICKS = getUptime();
	return OK;
}
