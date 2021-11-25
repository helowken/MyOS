#ifndef TYPE_H
#define TYPE_H

#include "sys/types.h"

typedef void (*taskFunc)();

typedef int proc_nr_t;		/* Process table entry number */
typedef short sys_id_t;		/* System process index */
typedef unsigned reg_t;		/* Machine register */
typedef struct {			/* Bitmap for System indexes */
	bitchunk_t chunk[BITMAP_CHUNKS(NR_SYS_PROCS)];
} SysMap;

typedef struct {
	proc_nr_t procNum;			/* Process number to use */
	taskFunc initialPC;			/* Start function for tasks */
	int flags;					/* Process flags */
	unsigned char quantum;		/* Quantum (tick count) */
	int priority;				/* Scheduling priority */
	int stackSize;				/* Stack size for tasks */
	short trapMask;				/* Allowed system call traps */
	bitchunk_t ipcTo;			/* Send mask protection */
	long callMask;				/* System call protection */
	char procName[P_NAME_LEN];	/* Name in process table */
} BootImage;

typedef struct {
	u16_t gs;
	u16_t fs;
	u16_t es;
	u16_t ds;
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

typedef struct {
	u16_t limitLow;
	u16_t baseLow;
	u8_t baseMiddle;
	u8_t access;		/* | P |    DPL    | S=1 | Data(0|E|W|A) / Code(1|C|R|A) | */
	u8_t granularity;	/* | G | D/B | L=0 | AVL | LIMIT | */
	u8_t baseHigh;
} SegDesc;

struct IrqHook;
typedef unsigned long irq_policy_t;
typedef unsigned long irq_id_t;
typedef int (*irq_handler_t)(struct IrqHook *);

typedef struct IrqHook {
	struct IrqHook *next;	/* Next hook in chain */
	irq_handler_t handler;	/* Interrupt handler */
	int irq;				/* IRQ vector number */
	int id;					/* Id of this hook */
	int procNum;			/* NONE if not in use */
	irq_id_t notifyId;		/* Id to return on interrupt */
	irq_policy_t policy;	/* Bit mask for policy */
} IrqHook;

#endif
