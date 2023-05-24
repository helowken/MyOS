#include "../system.h"
#include <stddef.h>

int doGetInfo(Message *msg) {
/* Request system information to be copied to caller's address space. This
 * call simply copies entire data structures to the caller.
 */
	size_t length;
	phys_bytes src;
	phys_bytes dst;
	int pNum, i;
	static Randomness copy;

	/* Set source address and length based on request type. */
	switch (msg->I_REQUEST) {
		case GET_MACHINE:
			length = sizeof(Machine);
			src = vir2Phys(&machine);
			break;
		case GET_KINFO:
			length = sizeof(KernelInfo);
			src = vir2Phys(&kernelInfo);
			break;
		case GET_IMAGE:
			length = sizeof(BootImage) * NR_BOOT_PROCS;
			src = vir2Phys(images);
			break;
		case GET_IRQHOOKS:
			length = sizeof(IrqHook) * NR_IRQ_HOOKS;
			src = vir2Phys(irqHooks);
			break;
		case GET_SCHEDINFO: 
			/* This is slightly complicated because we need two data structures
			 * at once, otherwise the scheduling information may be incorrect.
			 * Copy the queue heads and fall through to copy the process table.
			 */
			length = sizeof(Proc) * NR_SCHED_QUEUES;
			src = vir2Phys(readyProcHead);
			dst = nUMapLocal(msg->m_source, (vir_bytes) msg->I_VAL_PTR2, length);
			if (src == 0 || dst == 0)
			  return EFAULT;
			physCopy(src, dst, length);
			/* Fall through */
		case GET_PROCTAB:
			length = sizeof(Proc) * (NR_PROCS + NR_TASKS);
			src = vir2Phys(procTable);
			break;
		case GET_PRIVTAB:
			length = sizeof(Priv) * NR_SYS_PROCS;
			src = vir2Phys(privTable);
			break;
		case GET_PROC:
			pNum = (msg->I_VAL_LEN2 == SELF) ? msg->m_source : msg->I_VAL_LEN2;
			if (! isOkProcNum(pNum))
			  return EINVAL;	
			length = sizeof(Proc);
			src = vir2Phys(procAddr(pNum));
			break;
		case GET_MONPARAMS:
			src = kernelInfo.paramsBase;
			length = kernelInfo.paramsSize;
			break;
		case GET_RANDOMNESS:
			copy = kernelRandom;
			for (i = 0; i < RANDOM_SOURCES; ++i) {
				kernelRandom.bin[i].r_size = 0;		/* Invalidate random data */
				kernelRandom.bin[i].r_next = 0;
			}
			length = sizeof(Randomness);
			src = vir2Phys(&copy);
			break;
		case GET_KMESSAGES: 
			length = sizeof(KernelMsgs);
			src = vir2Phys(&kernelMsgs);
			break;
		case GET_BIOSBUFFER:
			// TODO
		default:
			return EINVAL;
	}

	/* Try to make the actual copy for the requested data. */
	if (msg->I_VAL_LEN > 0 && length > msg->I_VAL_LEN)
	  return E2BIG;
	pNum = msg->m_source;		/* Only caller can request copy */
	dst = nUMapLocal(pNum, (vir_bytes) msg->I_VAL_PTR, length);
	if (src == 0 || dst == 0)
	  return EFAULT;
	physCopy(src, dst, length);
	return OK;
}
