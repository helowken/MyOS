#ifndef _SYSLIB_H
#define	_SYSLIB_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

#ifndef _IPC_H
#include "minix/ipc.h"
#endif

#ifndef _DEVIO_H
#include "minix/devio.h"
#endif

#define SYSTASK	SYSTEM

/*=====================================================================*
 *					Minix system library                *
 *=====================================================================*/
int taskCall(int who, int sysCallNum, Message *msg);
int sysAbort(int how, ...);
int sysExec(int pNum, char *newSp, char *progName, vir_bytes initPc);
int sysFork(int parent, int child);
int sysNewMap(int pNum, MemMap *memMap);
int sysTrace(int req, int proc, long addr, long *data);
int sysNice(int pNum, int priority);

/* Clock functionality: get system times or (un)schedule an alarm call. */
int sysTimes(int pNum, clock_t *ptr);
int sysSetAlarm(clock_t expTime, int absTime);
int sysExit(int pNum);

int sysSegCtl(int *index, u16_t *segSel, vir_bytes *off, 
			phys_bytes physAddr, vir_bytes size);

/* Shorthands for sysStrDevIO() system call. */
#define sysInsb(port, pNum, buffer, count) \
	sysStrDevIO(DIO_INPUT, port, DIO_BYTE, pNum, buffer, count)
#define sysInsw(port, pNum, buffer, count) \
	sysStrDevIO(DIO_INPUT, port, DIO_WORD, pNum, buffer, count)
#define sysOutsb(port, pNum, buffer, count) \
	sysStrDevIO(DIO_OUTPUT, port, DIO_BYTE, pNum, buffer, count)
#define sysOutsw(port, pNum, buffer, count) \
	sysStrDevIO(DIO_OUTPUT, port, DIO_WORD, pNum, buffer, count)
int sysStrDevIO(int req, long port, int type, int pNum, void *buffer, int count);

/* Shorthands for sysGetInfo() system call. */
#define sysGetMonParams(v, vl)	sysGetInfo(GET_MONPARAMS, v, vl, 0, 0)
#define sysGetKernelMsgs(dst)	sysGetInfo(GET_KMESSAGES, dst, 0, 0, 0)
#define sysGetKernelInfo(dst)	sysGetInfo(GET_KINFO, dst, 0, 0, 0)
#define sysGetMachine(dst)		sysGetInfo(GET_MACHINE, dst, 0, 0, 0)
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

#define sysAbsCopy(srcPhys, dstPhys, bytes) \
	sysPhysCopy(NONE, PHYS_SEG, srcPhys, NONE, PHYS_SEG, dstPhys, bytes)
int sysPhysCopy(int srcProc, int srcSeg, vir_bytes srcVir,
		int dstProc, int dstSeg, vir_bytes dstVir, phys_bytes bytes);
int sysMemset(unsigned long pattern, phys_bytes base, phys_bytes bytes);

int sysVecOutb(PvBytePair *pair, int num);
int sysVecOutw(PvWordPair *pair, int num);
int sysVecOutl(PvLongPair *pair, int num);
int sysVecInb(PvBytePair *pair, int num);
int sysVecInw(PvWordPair *pair, int num);
int sysVecInl(PvLongPair *pair, int num);

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
