#ifndef CONFIG_H
#define	CONFIG_H

/* How many bytes for the kernel stack. Space allocated in mpx.S. */
#define K_STACK_BYTES	1024

#define P_NAME_LEN		8

#define NR_IRQ_HOOKS	16		/* Number of interrupt hooks */

#endif
