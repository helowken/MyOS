#ifndef _MINIX_CONFIG_H
#define _MINIX_CONFIG_h

#define	OS_RELEASE "3"
#define	OS_VERSION "1.0"

#include "sys_config.h"

#define NR_PROCS		_NR_PROCS
#define NR_SYS_PROCS	_NR_SYS_PROCS

/* DMA_SECTORS may be increased to speed up DMA based drivers. */
#define DMA_SECTORS		1	/* DMA buffer size (must be >= 1) */

/* The buffer cache should be made as large as you can afford. */
#define NR_BUFS			1280	/* Blocks in the buffer cache */
#define NR_BUF_HASH		2048	/* Size of buf hash table; MUST BE POWER OF 2 */

/* Number of controller tasks (/dev/cN device classes). */
#define NR_CTRLRS		2

/* Which process should receive diagnostics from the kernel and system?
 * Directly sending it to TTY only displays the output. Sending it to the
 * log driver will cause the diagnostics to be buffered and displayed.
 */
#define OUTPUT_PROC_NR	LOG_PROC_NR	/* TTY_PROC_NR or LOG_PROC_NR */

/* NR_CONS, NR_RS_LINES, and NR_PTYS determine the number of terminals the
 * system can handle.
 */
#define NR_CONS			4	/* # system consoles (1 to 8) */
#define NR_RS_LINES		4	/* # rs232 terminals (0 to 4) */
#define NR_PTYS			32	/* # pseudo terminals (0 to 64) */

#endif
