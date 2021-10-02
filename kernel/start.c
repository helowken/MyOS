#include "kernel.h"
#include "protect.h"
#include "proc.h"
#include "stdlib.h"
#include "string.h"

PUBLIC void cstart(U16_t cs, U16_t ds,	/* Kernel code and data segment. */
			U16_t mds,					/* Moniotr data/stack segment. */
			U16_t paramOffset,			/* Boot parameters offset. */
			U16_t paramSize)			/* Boot parameters size. */
{			
	//char params[128 * sizeof(char *)];	/* One sector size, 512 bytes. */
	//register char *value;				/* Value in (key = value) pair. */	
	extern char etext, end;

	kernelInfo.codeBase = seg2Phys(cs);
	kernelInfo.codeSize = (phys_bytes) &etext;		/* Size of code segment. */
	kernelInfo.dataBase = seg2Phys(ds);
	kernelInfo.dataSize = (phys_bytes) &end;		/* Size of data segment (data + bss). */

	/* Initialize protected mode descriptors. */
	protectInit();
}
