#include "kernel.h"
#include "proc.h"
#include "protect.h"



PUBLIC SegDesc gdt[GDT_SIZE];		/* Used in mpx.s */


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

/* 
 * Set up tables for protected mode.
 * All GDT slots are allocated at compile time.
 */
PUBLIC void protectInit() {
	//struct GateTable *gtp;
	
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

	if (0) {
		gateTable[0].gate();
	}
}
