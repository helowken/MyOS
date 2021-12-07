#ifndef CONFIG_H
#define	CONFIG_H

/* How many bytes for the kernel stack. Space allocated in mpx.S. */
#define K_STACK_BYTES	1024

/* Length of program names stored in the process table. */
#define P_NAME_LEN		8

/* Kernel diagnostics are written to a circular buffer. After each message,
 * a system server is notified and a copy of the buffer can be retrieved to
 * display the message. The buffers size can safely be reduced.
 */
#define KMESS_BUF_SIZE	256

#define NR_IRQ_HOOKS	16		/* Number of interrupt hooks */

/* Buffer to gather randomness. This is used to generate a random stream by
 * the MEMORY driver when reading from /dev/random
 */
#define RANDOM_ELEMENTS	32

#endif
