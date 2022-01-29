#ifndef _SYSLIB_H
#define	_SYSLIB_H

#include "sys/types.h"
#include "minix/ipc.h"

#define SYSTASK	SYSTEM

/*=====================================================================*
 *					Minix system library                *
 *=====================================================================*/
int taskCall(int who, int sysCallNum, Message *msg);
int sysTrace(int req, int proc, long addr, long *data);
int sysNice(int pNum, int priority);

/* Clock functionality: get system times or (un)schedule an alarm call. */
int sysTimes(int pNum, clock_t *ptr);
int sysSetAlarm(clock_t expTime, int absTime);
int sysExit(int pNum);

/* Shorthands for sysGetInfo() system call. */
#define sysGetMonParams(v, vl)	sysGetInfo(GET_MONPARAMS, v, vl, 0, 0)
#define sysGetKernelInfo(dst)	sysGetInfo(GET_KINFO, dst, 0, 0, 0)
#define sysGetProc(dst, num)	sysGetInfo(GET_PROC, dst, 0, 0, num)
#define sysGetImage(dst)		sysGetInfo(GET_IMAGE, dst, 0, 0, 0)
int sysGetInfo(int request, void *valPtr, int valLen, void *valPtr2, int valLen2);

/* Signal control. */
int sysKill(int pNum, int sigNum);
int sysSigSend(int pNum, SigMsg *sigCtx);
int sysSigReturn(int pNum, SigMsg *sigCtx);
int sysGetKernelSig(int *pNum, sigset_t *sigMap);
int sysEndKernelSig(int pNum);

/* Shorthands for sysVirCopy() and sysPhysCopy() system calls. */
#define sysDataCopy(srcProc, srcVir, dstProc, dstVir, bytes) \
	sysVirCopy(srcProc, D, srcVir, dstProc, D, dstVir, bytes)
#define sysTextCopy(srcProc, srcVir, dstProc, dstVir, bytes) \
	sysVirCopy(srcProc, T, srcVir, dstProc, T, dstVir, bytes)
int sysVirCopy(int srcProc, int srcSeg, vir_bytes srcVir, 
		int dstProc, int dstSeg, vir_bytes dstVir, phys_bytes bytes);

/* Shorthands for sysOut() system call. */
#define sysOutb(p, v)	sysOut((p), (unsigned long) (v), DIO_BYTE)
#define sysOutw(p, v)	sysOut((p), (unsigned long) (v), DIO_WORD)
#define sysOutl(p, v)	sysOut((p), (unsigned long) (v), DIO_LONG)
int sysOut(int port, unsigned long value, int type);

/* Shorthands for sysIn() system call. */
#define sysInb(p, v)	sysIn((p), (unsigned long *) (v), DIO_BYTE)
#define sysInw(p, v)	sysIn((p), (unsigned long *) (v), DIO_WORD)
#define sysInl(p, v)	sysIn((p), (unsigned long *) (v), DIO_LONG)
int sysIn(int port, unsigned long *value, int type);

/* Shorthands for sysIrqCtl() system call. */
#define sysIrqDisable(hookId) sysIrqCtl(IRQ_DISABLE, 0, 0, hookId)
#define sysIrqEnable(hookId)  sysIrqCtl(IRQ_ENABLE, 0, 0, hookId)
#define	sysIrqSetPolicy(irqVec, policy, hookId) \
	sysIrqCtl(IRQ_SET_POLICY, irqVec, policy, hookId)
#define sysIrqRmPolicy(irqVec, hookId) \
	sysIrqCtl(IRQ_RM_POLICY, irqVec, 0, hookId)
int sysIrqCtl(int request, int irqVec, int policy, int *irqHookId);

int sysUMap(int pNum, int seg, vir_bytes virAddr, vir_bytes bytes, 
			phys_bytes *physAddr);

#endif
