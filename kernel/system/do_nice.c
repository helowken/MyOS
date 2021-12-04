#include "../system.h"
#include "sys/resource.h"

int doNice(Message *msg) {
	int pNum, priority, newQueue;
	register Proc* rp;

	/* Extract the message parameters and do sanity checking. */
	pNum = msg->PR_PROC_NR;
	if (! isOkProcNum(pNum))
	  return EINVAL;	
	if (isKernelNum(pNum))
	  return EPERM;

	priority = msg->PR_PRIORITY;
	if (priority < PRIO_MIN || priority > PRIO_MAX)
	  return EINVAL;

	/* The priority is currently between PRIO_MIN and PRIO_MAX. We have to
	 * scale this between MIN_USER_Q and MAX_USER_Q.
	 */
	newQueue = MAX_USER_Q + (priority - PRIO_MIN) * (MIN_USER_Q - MAX_USER_Q + 1) / 
				(PRIO_MIN - PRIO_MAX + 1);
	if (newQueue < MAX_USER_Q)
	  newQueue = MAX_USER_Q;		
	else if (newQueue > MIN_USER_Q)
	  newQueue = MIN_USER_Q;

	/* Make sure the process is not running while changing its priority; the
	 * max_priority is the base priority. Put the process back in its new
	 * queue if it is runnable.
	 */
	rp = procAddr(pNum);
	lockDequeue(rp);
	rp->p_max_priority = rp->p_priority = newQueue;
	if (rp->p_rt_flags == 0)
	  lockEnqueue(rp);

	return OK;
}
