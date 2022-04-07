#include "kernel.h"
#include "protect.h"
#include "proc.h"
#include "stdlib.h"
#include "string.h"

static char *getValue(char *params, char *name) {
	register const size_t len = strlen(name);
	register char *envp;

	envp = params;
	while (*envp != '\0') {
		if (!strncmp(envp, name, len) && envp[len] == '=') 
		  return &envp[len + 1];

		while (*envp++ != '\0');
	}
	return NULL;
}

void cstart(u16_t cs, u16_t ds,	/* Kernel code and data segment. */
			u16_t mds,					/* Moniotr data/stack segment. */
			u16_t paramOffset,			/* Boot parameters offset. */
			u16_t paramSize)			/* Boot parameters size. */
{			
	char params[128 * sizeof(char *)];	/* One sector size, 512 bytes. */
	register char *value;				/* Value in (key = value) pair. */	
	extern char etext, end;

	kernelInfo.codeBase = seg2Phys(cs);
	kernelInfo.codeSize = (phys_bytes) &etext;		/* Size of code segment. */
	kernelInfo.dataBase = seg2Phys(ds);
	kernelInfo.dataSize = (phys_bytes) &end;		/* Size of data segment (data + bss). */

	/* Initialize protected mode descriptors. */
	protectInit();

	/* Copy the boot parameters to the local buffer. */
	kernelInfo.paramsBase = seg2Phys(mds) + paramOffset;
	kernelInfo.paramsSize = MIN(paramSize, sizeof(params) - 2);
	physCopy(kernelInfo.paramsBase, vir2Phys(params), kernelInfo.paramsSize);

	/* Record miscellaneous information for user-space servers. */
	kernelInfo.numProcs = NR_PROCS;
	kernelInfo.numTasks = NR_TASKS;
	strncpy(kernelInfo.release, OS_RELEASE, sizeof(kernelInfo.release));
	kernelInfo.release[sizeof(kernelInfo.release) - 1] = '\0';
	strncpy(kernelInfo.version, OS_VERSION, sizeof(kernelInfo.version));
	kernelInfo.version[sizeof(kernelInfo.version) - 1] = '\0';
	kernelInfo.procTableAddr = (vir_bytes) procTable;
	kernelInfo.kernelMemBase = vir2Phys(0);
	kernelInfo.kernelMemSize = (phys_bytes) &end;

	/* Processor */
	machine.processor = atoi(getValue(params, "processor"));

	/* XT, AT or MCA bus? */
	value = getValue(params, "bus");
	if (value == NULL || strcmp(value, "at") == 0)
	  machine.pc_at = true;
	else if (strcmp(value, "mca") == 0)
	  machine.pc_at = machine.ps_mca = true;

	/* Type of VDU */
	value = getValue(params, "video");
	if (strcmp(value, "ega") == 0)
	  machine.vdu_ega = true;
	else if (strcmp(value, "vga") == 0)
	  machine.vdu_ega = machine.vdu_vga = true;
}
