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
	vir_clicks offset;
} MemMap;

typedef struct {
	int inUse;						/* Entry in use, unless zero */
	phys_clicks physAddr;			/* Physical address */
	vir_clicks len;					/* Length */
} FarMem;

typedef struct {
	int pNum;
	int segment;
	vir_bytes offset;
} VirAddr;

typedef struct {
	phys_bytes codeBase;			/* Base of kernel code */
	phys_bytes codeSize;			/* Size of kernel code */
	phys_bytes dataBase;			/* Base of kernel data */
	phys_bytes dataSize;			/* Size of kernel data */
	vir_bytes procTableAddr;		/* Virtual address of process table */
	phys_bytes kernelMemBase;		/* Kernel memory layout (/dev/kmem) */
	phys_bytes kernelMemSize;
	phys_bytes bootDevBase;			/* Boot Device from boot image (/dev/boot) */
	phys_bytes bootDevSize;
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

/* PM passes the address of a structure of this type to KERNEL when
 * sysSendSig() is invoked as part of the signal catching mechanism.
 * The structure contains all the information that KERNEL needs to build
 * the signal stack.
 */
typedef struct {
	int sm_sig_num;		/* Signal number being caught */
	unsigned long sm_mask;		/* Mask to restore when handler returns */
	vir_bytes sm_sig_handler;	/* Address of handler */
	vir_bytes sm_sig_return;	/* Address of _sigReturn in C library */
	vir_bytes sm_stack_ptr;		/* User stack pointer */
} SigMsg;

typedef struct {
	vir_bytes iov_addr;		/* Address of an I/O buffer */
	vir_bytes iov_size;		/* Sizeof an I/O buffer */
} IOVec;

#endif
