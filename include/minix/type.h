#ifndef _MINIX_TYPE_H
#define _MINIX_TYPE_H

#ifndef _MINIX_SYS_CONFIG_H
#include "minix/sys_config.h"
#endif

#ifndef _TYPES_H
#include "sys/types.h"
#endif

typedef enum { false, true } bool;

typedef unsigned reg_t;		/* Machine register */

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

typedef struct StackFrame {
	reg_t gs;
	reg_t fs;
	reg_t es;
	reg_t ds;
	reg_t edi;			/* edi through ecx are not accessed in C */
	reg_t esi;			/* Order is to match pusha/popa */
	reg_t ebp;			
	reg_t temp;			
	reg_t ebx;
	reg_t edx;
	reg_t ecx;
	reg_t eax;			/* gs through eax are all pushed by save() in assembly */
	reg_t retAddr;		/* Return address for save() in assembly */
	reg_t pc;			/* pc(eip), cs, psw(eflags) are pushed by interrupt */
	reg_t cs;
	reg_t psw;			/* psw (program status word) = eflags */
	reg_t esp;			/* esp, ss are pushed by processor when a stack switched */
	reg_t ss;
} StackFrame;

#endif
