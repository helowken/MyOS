#ifndef IPC_H
#define IPC_H

#include "minix/com.h"

/* System call numbers that are passed when trapping to the kernel. The 
 * numbers are carefully defined so that it can easily be seen (based on
 * the bits that are on) which checks should be done in sys_call().
 */
#define SEND		1		/* 0 0 0 1: blocking send */
#define	RECEIVE		2		/* 0 0 1 0: blocking receive */
#define SENDREC		3		/* 0 0 1 1: SEND + RECEIVE */
#define NOTIFY		4		/* 0 1 0 0: nonblocking notify */
#define ECHO		8		/* 1 0 0 0: echo a message */

#endif
