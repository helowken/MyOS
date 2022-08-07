#include "../system.h"

int doNewMap(Message *msg) {
/* Handle sysNewMap(). Fetch the memory map from PM. */

	register Proc *rp;
	int caller;
	MemMap *memMap;
	phys_bytes srcPhys;
	int oldFlags;

	/* Extract message parameters and copy new memory map from PM. */
	caller = msg->m_source;
	memMap = (MemMap *) msg->PR_MEM_PTR;
	if (! isOkProcNum(msg->PR_PROC_NR))
	  return EINVAL;
	if (isKernelNum(msg->PR_PROC_NR))
	  return EPERM;
	rp = procAddr(msg->PR_PROC_NR);

	/* Copy the memory map from PM. */
	srcPhys = umapLocal(procAddr(caller), D, (vir_bytes) memMap,
					sizeof(rp->p_memmap));
	if (srcPhys == 0)
	  return EFAULT;
	physCopy(srcPhys, vir2Phys(rp->p_memmap), 
				(phys_bytes) sizeof(rp->p_memmap));
	allocSegments(rp);
	
	oldFlags = rp->p_rts_flags;
	rp->p_rts_flags &= ~NO_MAP;
	if (oldFlags != 0 && rp->p_rts_flags == 0) 
	  lockEnqueue(rp);

	return OK;
}
