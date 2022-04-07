#ifndef IPC_H
#define IPC_H

#include "minix/com.h"

/* Masks and flags for system calls. */
#define SYSCALL_FUNC	0x0F	/* Mask for system call function */
#define SYSCALL_FLAGS	0xF0	/* Mask for system call flags */
#define NON_BLOCKING	0x10	/* Prevent blocking, return error */

/* System call numbers that are passed when trapping to the kernel. The 
 * numbers are carefully defined so that it can easily be seen (based on
 * the bits that are on) which checks should be done in sys_call().
 */
#define SEND			1		/* 0 0 0 1: blocking send */
#define	RECEIVE			2		/* 0 0 1 0: blocking receive */
#define SENDREC			3		/* 0 0 1 1: SEND + RECEIVE */
#define NOTIFY			4		/* 0 1 0 0: nonblocking notify */
#define ECHO			8		/* 1 0 0 0: echo a message */

/* The following bit masks determine what checks that should be done. */
#define CHECK_DST		0x05	/* 0 1 0 1: validate message destination */
#define CHECK_SRC		0x02	/* 0 0 1 0: validate message source */

#endif
