#ifndef _SYSLIB_H
#define	_SYSLIB_H

#include "sys/types.h"
#include "minix/ipc.h"

#define SYSTASK	SYSTEM

int taskCall(int who, int sysCallNum, Message *msg);

/* Clock functionality: get system times or (un)schedule an alarm call. */
int sysTimes(int pNum, clock_t *ptr);
int sysSetAlarm(clock_t expTime, int absTime);

/* Shorthands for sys_getinfo() system call. */
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

#endif
