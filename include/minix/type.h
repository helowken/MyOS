#ifndef _MINIX_TYPE_H
#define _MINIX_TYPE_H

#ifndef _MINIX_SYS_CONFIG_H
#include "minix/sys_config.h"
#endif

#ifndef _TYPES_H
#include "sys/types.h"
#endif

typedef enum { false, true } bool;

/* Type definition. */
typedef unsigned long phys_bytes;	/* Physical addr/length in bytes */
typedef unsigned int phys_clicks;	/* Physical addr/length in clicks */
typedef unsigned int vir_bytes;		/* Virtual addr/length in bytes */
typedef unsigned int vir_clicks;	/* Virtual addr/length in clicks */

typedef struct {
	phys_clicks physAddr;			/* Physical address */
	vir_clicks virAddr;				/* Virtual address */
	vir_clicks len;					/* Length */
} MemMap;

typedef struct {
	phys_bytes codeBase;			/* Base of kernel code */
	phys_bytes codeSize;			/* Size of kernel code */
	phys_bytes dataBase;			/* Base of kernel data */
	phys_bytes dataSize;			/* Size of kernel data */
	vir_bytes procTableAddr;		/* Virtual address of process table */
	phys_bytes kernelMemBase;		/* Kernel memory layout */
	phys_bytes kernelMemSize;
	phys_bytes paramsBase;			/* Parameters passed by boot monitor */
	phys_bytes paramsSize;
	int numProcs;					/* Number of user processes */
	int numTasks;					/* Number of kernel tasks */
	char release[6];				/* Kernel release number */
	char version[6];				/* Kernel version number */
} KernelInfo;

typedef struct {
	bool pc_at;
	bool ps_mca;
	int processor;
	bool vdu_ega;
	bool vdu_vga;
} Machine;

#endif
