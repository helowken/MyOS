#include "kernel.h"
#include "proc.h"
#include "protect.h"


typedef struct {
	char limit[sizeof(u16_t)];
	char base[sizeof(u32_t)];
} DescTablePtr;

typedef struct {
	u16_t offsetLow;
	u16_t selector;
	u8_t pad;			/* |000|XXXXX| interrupt gate & trap gate, |XXXXXXXX| task gate */
	u8_t pDplType;		/* |P|DPL|0|TYPE| */
	u16_t offsetHigh;
} GateDesc;


PUBLIC SegDesc gdt[GDT_SIZE];		/* Used in mpx.s */
PUBLIC GateDesc idt[IDT_SIZE];		/* Zero-init so none present */


/* 
 * Return the base address of a segment, with a 386 segment selector.
 */
PUBLIC phys_bytes seg2Phys(U16_t segSelector) {
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

/* 
 * Set up tables for protected mode.
 * All GDT slots are allocated at compile time.
 */
PUBLIC void protectInit() {
	//struct GateTable *gtp;
	DescTablePtr *dtp;
	
	static struct GateTable {
		void (*gate)();
		unsigned char vectorNum;
		unsigned char privilege;
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
		{ coprError, COPROC_ERR_VECTOR, INTR_PRIVILEGE }
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

	if (0) {
		gateTable[0].gate();
	}
}

