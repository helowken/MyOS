#ifndef _SYSLIB_H
#define	_SYSLIB_H

#include "sys/types.h"
#include "minix/ipc.h"

#define SYSTASK	SYSTEM

int taskCall(int who, int sysCallNum, Message *msg);

/* Shorthands for sys_getinfo() system call. */
#define sysGetMonParams(v, vl)	sysGetInfo(GET_MONPARAMS, v, vl, 0, 0)

int sysGetInfo(int request, void *valPtr, int valLen, void *valPtr2, int valLen2);

#endif
