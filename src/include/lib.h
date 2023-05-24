#ifndef _LIB_H
#define _LIB_H

#define _POSIX_SOURCE	1
#define _MINIX			1

#include <minix/config.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>

#include <minix/const.h>
#include <minix/com.h>
#include <minix/type.h>
#include <minix/callnr.h>

#include <minix/ipc.h>

#define MM		PM_PROC_NR
#define FS		FS_PROC_NR

int syscall(int who, int callNum, Message *msg);
void _loadName(const char *name, Message *msg);

#endif
