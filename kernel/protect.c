#include "kernel.h"
#include "proc.h"
#include "protect.h"


#define INT_GATE_TYPE	(INT_286_GATE | DESC_386_BIT)
#define TSS_TYPE		(AVL_286_TSS | DESC_386_BIT)


typedef struct {
	char limit[sizeof(u16_t)];
	char base[sizeof(u32_t)];
} DescTablePtr;

typedef struct {
	u16_t offsetLow;
	u16_t selector;
	u8_t pad;			/* |000|XXXXX| interrupt gate & trap gate, |XXXXXXXX| task gate */
	u8_t dplType;		/* |P|DPL|0|TYPE| */
	u16_t offsetHigh;
} GateDesc;

typedef struct {
	reg_t preTaskLink;
	reg_t esp0;
	reg_t ss0;
	reg_t esp1;
	reg_t ss1;
	reg_t esp2;
	reg_t ss2;
	reg_t cr3;			/* PDBR */
	reg_t eip;
	reg_t eflags;
	reg_t eax;
	reg_t ecx;
	reg_t edx;
	reg_t ebx;
	reg_t esp;
	reg_t ebp;
	reg_t esi;
	reg_t edi;
	reg_t es;
	reg_t cs;
	reg_t ss;
	reg_t ds;
	reg_t fs;
	reg_t gs;
	reg_t ldtSel;
	u16_t trap;			/* T (debug trap) flag (bit 0) */
	u16_t ioBaseAddr;
} TSS;					/* Task state segment */


SegDesc gdt[GDT_SIZE];		/* Used in mpx.s */
static GateDesc idt[IDT_SIZE];		/* Zero-init so none present */
TSS tss;						

/* 
 * Return the base address of a segment, with a 386 segment selector.
 */
phys_bytes seg2Phys(U16_t segSelector) {
	phys_bytes base;
	SegDesc *segDp;

	segDp = &gdt[segSelector >> 3];					/* Each descriptor is 8 bytes. */
	base = ((u32_t) segDp->baseLow) |				/* Low 16 bits of base. */
			((u32_t) segDp->baseMiddle << 16) |		/* Middle 8 bits of base. */
			((u32_t) segDp->baseHigh << 24);		/* High 8 bits of base. */

	return base;
}

/* Fill in the fields (base, limit and granularity) of a descriptor */
static void setupSegDesc(register SegDesc *sdPtr, phys_bytes base, vir_bytes size) {
	sdPtr->baseLow = base;
	sdPtr->baseMiddle = base >> BASE_MIDDLE_SHIFT;
	sdPtr->baseHigh = base >> BASE_HIGH_SHIFT;

	--size;		/* Convert to a limit (limit + 1 = size), 0 size means 4 GB */
	if (size > BYTE_GRAN_MAX) {
		/* Set G = 1, then size can range from 4 KB to 4 GB, in 4 KB increments. */
		sdPtr->limitLow = size >> PAGE_GRAN_SHIFT;	
		sdPtr->granularity = GRANULAR | (size >> (PAGE_GRAN_SHIFT + GRANULARITY_SHIFT));
	} else {
		/* G = 0, size can range from 1 byte to 1 MB, in byte increments. */
		sdPtr->limitLow = size;
		sdPtr->granularity = size >> GRANULARITY_SHIFT;
	}
	sdPtr->granularity |= DEFAULT;	/* Means BIG for data segment (D/B) */
}

static void initCodeSeg(register SegDesc *sdPtr, phys_bytes base, vir_bytes size, int privilege) {
	setupSegDesc(sdPtr, base, size);
	/* CONFORMING = 0, ACCESSED = 0 */
	sdPtr->access = (privilege << DPL_SHIFT) | PRESENT | SEGMENT | EXECUTABLE | READABLE;
}

static void initDataSeg(register SegDesc *sdPtr, phys_bytes base, vir_bytes size, int privilege) {
	setupSegDesc(sdPtr, base, size);
	/* EXECUTABLE = 0, EXPAND_DOWN = 0, ACCESSED = 0 */
	sdPtr->access = (privilege << DPL_SHIFT) | PRESENT | SEGMENT | WRITABLE;
}

static void initGate(u8_t vectorNum, vir_bytes offset, u8_t dplType) {
	register GateDesc *idp;

	idp = &idt[vectorNum];
	idp->offsetLow = offset;
	idp->offsetHigh = offset >> OFFSET_HIGH_SHIFT;
	idp->selector = CS_SELECTOR;
	idp->dplType = dplType;
}

/* 
 * Set up tables for protected mode.
 * All GDT slots are allocated at compile time.
 */
void protectInit() {
	struct GateTable *gtp, *gtpEnd;
	DescTablePtr *dtp;
	unsigned ldtIndex;
	register Proc *rp;
	
	static struct GateTable {
		void (*gate)();
		u8_t vectorNum;
		u8_t privilege;
	} gateTable[] = {
		{ divideError, DIVIDE_VECTOR, INTR_PRIVILEGE },
		{ singleStepException, DEBUG_VECTOR, INTR_PRIVILEGE },
		{ nmi, NMI_VECTOR, INTR_PRIVILEGE },
		{ breakpointException, BREAKPOINT_VECTOR, USER_PRIVILEGE },
		{ overflow, OVERFLOW_VECTOR, USER_PRIVILEGE },
		{ boundsCheck, BOUNDS_VECTOR, INTR_PRIVILEGE },
		{ invalOpcode, INVAL_OP_VECTOR, INTR_PRIVILEGE },
		{ coprNotAvailable, COPROC_NOT_VECTOR, INTR_PRIVILEGE },
		{ doubleFault, DOUBLE_FAULT_VECTOR, INTR_PRIVILEGE },
		{ coprSegOverrun, COPROC_SEG_VECTOR, INTR_PRIVILEGE },
		{ invalTss, INVAL_TSS_VECTOR, INTR_PRIVILEGE },
		{ segmentNotPresent, SEG_NOT_VECTOR, INTR_PRIVILEGE },
		{ stackException, STACK_FAULT_VECTOR, INTR_PRIVILEGE },
		{ generalProtection, PROTECTION_VECTOR, INTR_PRIVILEGE },
		{ pageFault, PAGE_FAULT_VECTOR, INTR_PRIVILEGE },
		{ coprError, COPROC_ERR_VECTOR, INTR_PRIVILEGE },
		{ hwint00, VECTOR( 0), INTR_PRIVILEGE },
		{ hwint01, VECTOR( 1), INTR_PRIVILEGE },
		{ hwint02, VECTOR( 2), INTR_PRIVILEGE },
		{ hwint03, VECTOR( 3), INTR_PRIVILEGE },
		{ hwint04, VECTOR( 4), INTR_PRIVILEGE },
		{ hwint05, VECTOR( 5), INTR_PRIVILEGE },
		{ hwint06, VECTOR( 6), INTR_PRIVILEGE },
		{ hwint07, VECTOR( 7), INTR_PRIVILEGE },
		{ hwint08, VECTOR( 8), INTR_PRIVILEGE },
		{ hwint09, VECTOR( 9), INTR_PRIVILEGE },
		{ hwint10, VECTOR(10), INTR_PRIVILEGE },
		{ hwint11, VECTOR(11), INTR_PRIVILEGE },
		{ hwint12, VECTOR(12), INTR_PRIVILEGE },
		{ hwint13, VECTOR(13), INTR_PRIVILEGE },
		{ hwint14, VECTOR(14), INTR_PRIVILEGE },
		{ hwint15, VECTOR(15), INTR_PRIVILEGE },
		{ s_call, SYS386_VECTOR, USER_PRIVILEGE },
		{ level0_call, LEVEL0_VECTOR, TASK_PRIVILEGE }
	};

	/* Build gdt and idt pointers in GDT where the BIOS expects them. */
	dtp = (DescTablePtr *) &gdt[GDT_INDEX];
	* (u16_t *) dtp->limit = (sizeof gdt) - 1;
	* (u32_t *) dtp->base = vir2Phys(gdt);

	dtp = (DescTablePtr *) &gdt[IDT_INDEX];
	* (u16_t *) dtp->limit = (sizeof idt) - 1;
	* (u32_t *) dtp->base = vir2Phys(idt);

	/* Build segment descriptors for tasks and interrupt handlers. */
	initCodeSeg(&gdt[CS_INDEX], kernelInfo.codeBase, kernelInfo.codeSize, INTR_PRIVILEGE);
	initDataSeg(&gdt[DS_INDEX], kernelInfo.dataBase, kernelInfo.dataSize, INTR_PRIVILEGE);
	initDataSeg(&gdt[ES_INDEX], 0, 0, TASK_PRIVILEGE);

	/* Build local descriptors in GDT for LDT's in process table.
	 * The LDT's are allocated at compile time in the process table, and
	 * initialized whenever a process' map is initialized or changed.
	 */
	for (rp = BEG_PROC_ADDR, ldtIndex = FIRST_LDT_INDEX; rp < END_PROC_ADDR; ++rp, ++ldtIndex) {
		initDataSeg(&gdt[ldtIndex], vir2Phys(rp->p_ldt), sizeof(rp->p_ldt), INTR_PRIVILEGE);
		gdt[ldtIndex].access = PRESENT | LDT;
		rp->p_ldt_sel = ldtIndex * DESC_SIZE;
	}

	/* Build main TSS.
	 * This is used only to record the stack pointer to be used after an interrupt.
	 * The pointer is set up so that an interrupt automatically saves the current
	 * process's registers eip:cs:eflags:esp:ss in the correct slots in the process table.
	 */
	tss.ss0 = DS_SELECTOR;
	initDataSeg(&gdt[TSS_INDEX], vir2Phys(&tss), sizeof(tss), INTR_PRIVILEGE);
	gdt[TSS_INDEX].access = PRESENT | (INTR_PRIVILEGE << DPL_SHIFT) | TSS_TYPE;

	/* Build descriptors for interrupt gates in IDT. */
	for (gtp = &gateTable[0], gtpEnd = arrayLimit(gateTable); gtp < gtpEnd; ++gtp) {
		initGate(gtp->vectorNum, (vir_bytes) gtp->gate, 
					PRESENT | INT_GATE_TYPE | (gtp->privilege << DPL_SHIFT));
	}

	/* Complete building of main TSS. */
	tss.ioBaseAddr = sizeof(tss);
}

void allocSegments(Proc *rp) {
/* This is called at system initialization from main() and by doNewMap().
 * The code has a separate function because of all hardware-dependencies.
 * Note that IDLE is part of the kernel and gets TASK_PRIVILEGE here.
 */
	phys_bytes codeBytes;
	phys_bytes dataBytes;
	int privilege;

	codeBytes = rp->p_memmap[T].len << CLICK_SHIFT;
	dataBytes = (phys_bytes) (rp->p_memmap[S].virAddr + rp->p_memmap[S].len) << CLICK_SHIFT;
	privilege = isKernelProc(rp) ? TASK_PRIVILEGE : USER_PRIVILEGE;	/* Both need to switch stack. */
	initCodeSeg(&rp->p_ldt[CS_LDT_INDEX], rp->p_memmap[T].physAddr << CLICK_SHIFT,
				codeBytes, privilege);
	initDataSeg(&rp->p_ldt[DS_LDT_INDEX], rp->p_memmap[D].physAddr << CLICK_SHIFT,
				dataBytes, privilege);
	rp->p_reg.cs = (CS_LDT_INDEX * DESC_SIZE) | T1 | privilege; 
	rp->p_reg.gs = 
	rp->p_reg.fs = 
	rp->p_reg.ss = 
	rp->p_reg.es = 
	rp->p_reg.ds = (DS_LDT_INDEX * DESC_SIZE) | T1 | privilege;
}




