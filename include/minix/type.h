#ifndef _MINIX_TYPE_H
#define _MINIX_TYPE_H

#ifndef _MINIX_SYS_CONFIG_H
#include "minix/sys_config.h"
#endif

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* Type definition. */
typedef unsigned long phys_bytes;		/* Physical addr/length in bytes */

typedef struct {
	phys_bytes codeBase;				/* Base of kernel code. */
	phys_bytes codeSize;				/* Size of kernel code. */
	phys_bytes dataBase;				/* Base of kernel data. */
	phys_bytes dataSize;				/* Size of kernel data. */

} KernelInfo;

typedef struct {
	int pc_at;
	int ps_mca;
} Machine;

#endif
