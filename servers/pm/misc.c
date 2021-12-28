#include "pm.h"
#include "minix/callnr.h"
#include "signal.h"
#include "sys/resource.h"
#include "minix/com.h"
#include "string.h"
#include "mproc.h"
#include "param.h"

int doGetSetPriority() {
	int which, argWho, priority;
	int pNum;
	MProc *rmp;

	which = inputMsg.m1_i1;
	argWho = inputMsg.m1_i2;
	priority = inputMsg.m1_i3;		/* For SETPRIORITY */

	/* Only support PRIO_PROCESS for now. */
	if (which != PRIO_PROCESS)
	  return EINVAL;

	if (argWho == 0)
	  pNum = who;
	else if ((pNum = getPNumFromPid(argWho)) < 0)
	  return ESRCH;
	
	rmp = &mprocTable[pNum];

	if (currMp->mp_euid != SUPER_USER &&
		currMp->mp_euid != rmp->mp_euid &&
		currMp->mp_euid != rmp->mp_ruid)
	  return EPERM;

	/* If GET, that's it. */
	if (callNum == GETPRIORITY)
	  return rmp->mp_nice - PRIO_MIN;

	/* Only root is allowed to reduce the nice level. */
	if (rmp->mp_nice > priority && currMp->mp_euid != SUPER_USER)
	  return EACCES;

	/* We're SET, and it's allowed. Do it and tell kernel. */
	rmp->mp_nice = priority;
	return sysNice(pNum, priority);
}

int doGetProcNum() {
	register MProc *rmp;
	static char searchKey[PROC_NAME_LEN + 1];
	int keyLen;
	int s;
	int pNum = -1;

	if (inputMsg.proc_id >= 0) {	/* Lookup process by pid */
		for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
			if ((rmp->mp_flags & IN_USE) && 
					(rmp->mp_pid == inputMsg.proc_id)) {
				pNum = (int) (rmp - mprocTable);
				break;
			}
		}
	} else if (inputMsg.name_len > 0) {		/* Lookup process by name */
		keyLen = MIN(PROC_NAME_LEN, inputMsg.name_len);
		if ((s = sysDataCopy(who, (vir_bytes) inputMsg.addr, 
						SELF,(vir_bytes) searchKey, keyLen)) != OK)
		  return s;
		searchKey[keyLen] = '\0';		/* Terminate for safety */
		for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
			if ((rmp->mp_flags & IN_USE) &&
					strncmp(rmp->mp_name, searchKey, keyLen) == 0) {
				pNum = (int) (rmp - mprocTable);
				break;
			}
		}
	} else {
		pNum = who;
	}
	if (pNum == -1) 
	  return ESRCH;
	currMp->mp_reply.proc_num = pNum;
	return OK;
}
