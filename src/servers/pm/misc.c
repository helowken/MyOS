#include "pm.h"
#include "minix/callnr.h"
#include "signal.h"
#include "sys/svrctl.h"
#include "sys/resource.h"
#include "minix/com.h"
#include "string.h"
#include "mproc.h"
#include "param.h"

int doAllocMem() {
	vir_clicks memClicks;
	phys_clicks memBase;

	memClicks = (inMsg.m_mem_size + CLICK_SIZE - 1) >> CLICK_SHIFT;
	memBase = allocMemory(memClicks);
	if (memBase == NO_MEM)
	  return ENOMEM;
	currMp->mp_reply.m_mem_base = (phys_bytes) (memBase << CLICK_SHIFT);
	return OK;
}

int doGetSetPriority() {
	int which, argWho, priority;
	int pNum;
	MProc *rmp;

	which = inMsg.m1_i1;
	argWho = inMsg.m1_i2;
	priority = inMsg.m1_i3;		/* For SETPRIORITY */

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

	if (inMsg.m_proc_id >= 0) {	/* Lookup process by pid */
		for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
			if ((rmp->mp_flags & IN_USE) && 
					(rmp->mp_pid == inMsg.m_proc_id)) {
				pNum = (int) (rmp - mprocTable);
				break;
			}
		}
	} else if (inMsg.m_name_len > 0) {		/* Lookup process by name */
		keyLen = MIN(PROC_NAME_LEN, inMsg.m_name_len);
		if ((s = sysDataCopy(who, (vir_bytes) inMsg.m_addr, 
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
	currMp->mp_reply.m_proc_num = pNum;
	return OK;
}

int doGetSysInfo() {
	vir_bytes srcAddr, dstAddr;
	KernelInfo kernelInfo;
	size_t len;
	int s;

	switch (inMsg.m_info_what) {
		case SI_KINFO:		/* Kernel info is obtained via PM */
			sysGetKernelInfo(&kernelInfo);
			srcAddr = (vir_bytes) &kernelInfo;
			len = sizeof(KernelInfo);
			break;
		case SI_PROC_ADDR:
			srcAddr = (vir_bytes) &mprocTable[0];
			len = sizeof(MProc);
			break;
		case SI_PROC_TAB:
			srcAddr = (vir_bytes) mprocTable;
			len = sizeof(MProc) * NR_PROCS;
			break;
		default:
			return EINVAL;
	}

	dstAddr = (vir_bytes) inMsg.m_info_where;
	if ((s = sysDataCopy(SELF, srcAddr, who, dstAddr, len)) != OK)
	  return s;
	return OK;
}

int doSvrCtl() {
	int s, req;
	vir_bytes ptr;
#define MAX_LOCAL_PARAMS 2
	static struct {
		char name[30];
		char value[30];
	} localParamOverrides[MAX_LOCAL_PARAMS];
	static int localParams = 0;

	req = inMsg.m_svrctl_req;
	ptr = (vir_bytes) inMsg.m_svrctl_argp;

	/* Is the request indeed for the MM? */
	if (((req >> 8) & 0xFF) != 'M')
	  return EINVAL;

	/* Control operations local to the PM. */
	switch (req) { 
		case MM_SET_PARAM:
		case MM_GET_PARAM: {
			SysGetEnv sysGetEnv;
			char searchKey[64];
			char *valStart;
			size_t valLen;
			size_t copyLen;

			/* Copy SysGetEnv struct to PM. */
			if (sysDataCopy(who, ptr, SELF, (vir_bytes) &sysGetEnv, 
						sizeof(sysGetEnv)) != OK)
			  return EFAULT;
			
			/* Set a param override? */
			if (req == MM_SET_PARAM) {
				if (localParams >= MAX_LOCAL_PARAMS)
				  return ENOSPC;
				if (sysGetEnv.keyLen <= 0 || 
					sysGetEnv.keyLen >= sizeof(localParamOverrides[localParams].name) ||
					sysGetEnv.valLen <= 0 ||
					sysGetEnv.valLen >= sizeof(localParamOverrides[localParams].value))
				  return EINVAL;

				if ((s = sysDataCopy(who, (vir_bytes) sysGetEnv.key,
							SELF, (vir_bytes) localParamOverrides[localParams].name,
							sysGetEnv.keyLen)) != OK)
				  return s;
				if ((s = sysDataCopy(who, (vir_bytes) sysGetEnv.val,
							SELF, (vir_bytes) localParamOverrides[localParams].value,
							sysGetEnv.valLen)) != OK)
				  return s;

				localParamOverrides[localParams].name[sysGetEnv.keyLen] = 0;
				localParamOverrides[localParams].value[sysGetEnv.valLen] = 0;
				++localParams;

				return OK;
			}

			if (sysGetEnv.keyLen == 0) {	/* Copy all parameters */
				valStart = monitorParams;
				valLen = sizeof(monitorParams);
			} else {	/* Lookup value for key */
				int p;
				/* Try to get a copy of the requested key. */
				if (sysGetEnv.keyLen > sizeof(searchKey))
				  return EINVAL;
				if ((s = sysDataCopy(who, (vir_bytes) sysGetEnv.key,
							SELF, (vir_bytes) searchKey, sysGetEnv.keyLen)) != OK)
				  return s;

				/* Make sure key is null-terminated and lookup value.
				 * First check local overrides.
				 */
				searchKey[sysGetEnv.keyLen - 1] = 0;
				for (p = 0; p < localParams; ++p) {
					if (!strcmp(searchKey, localParamOverrides[p].name)) {
						valStart = localParamOverrides[p].value;
						break;
					}
				}
				if (p >= localParams && (valStart = findParam(searchKey)) == NULL)
				  return ESRCH;
				valLen = strlen(valStart) + 1;
			}

			/* 	See if it fits in the client's buffer. */
			if (valLen >= sysGetEnv.valLen)
			  return E2BIG;

			/* Value found, make the actual copy (as far as possible). */
			copyLen = MIN(valLen, sysGetEnv.valLen);
			if ((s = sysDataCopy(SELF, (vir_bytes) valStart,
						who, (vir_bytes) sysGetEnv.val, copyLen)) != OK)
			  return s;

			return OK;
		}
		case MM_SWAP_ON:
		case MM_SWAP_OFF:
			//TODO
		default:
			return EINVAL;
	}
}

#define REBOOT_CODE		"delay; boot"
int doReboot() {
	printf("===TODO pm reboot\n");
	return 0;
}

