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

#endif
