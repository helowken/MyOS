#include "at_wini.h"
#include "../libpci/pci.h"

#include "minix/sysutil.h"

#define MAX_DRIVES			8
#define COMPAT_DRIVES		4

static long wInstance = 0;

struct Wini {		/* Main drive struct, one entry per drive */
	unsigned state;			/* Drive state: deaf, initialized, dead */
	unsigned wStatus;		/* Device status register */
	unsigned baseCmdReg;	/* Command base register */
	unsigned baseCtlReg;	/* Control base register */
	unsigned irq;			/* Interrupt request line */
	unsigned irqMask;		/* 1 << irq */
	unsigned irqNeedAck;	/* irq needs to be acknowledged */
	int irqHookId;			/* Id of IrqHook at the kernel */
	int lba48;				/* Supports lba48 */
	unsigned lCylinders;	/* logical number of cylinders (BIOS) */
	unsigned lHeads;		/* logical number of heads */
	unsigned lSectors;		/* logical number of sectors per track */
	unsigned pCylinders;	/* physical number of cylinders (translated) */
	unsigned pHeads;		/* physical number of heads */
	unsigned pSectors;		/* physical number of sectors per track */
	unsigned precomp;		/* Write precompensation cylinder / 4 */
	unsigned maxCount;		/* Max request for this drive */
	unsigned openCount;		/* In-use count */
};
typedef struct Wini	Wini;
static Wini wini[MAX_DRIVES];

static int wDrive;				/* Selected drive */

static char *wName() {
	static char name[] = "AT-D0";

	name[4] = '0' + wDrive;
	return name;
}

static void initParams() {
	u16_t pairVal[2]; 
	unsigned int vector, size;
	int drive, numDrives;
	Wini *wn;
#define PARAMS_LEN	16
	u8_t params[PARAMS_LEN];
	int s;

	/* TODO Boot variables. */


	if (wInstance == 0) {
		/* Get the number of drives from the BIOS data area */
		if ((s = sysVirCopy(SELF, BIOS_SEG, NR_HD_DRIVES_ADDR,
				SELF, D, (vir_bytes) params, NR_HD_DRIVES_SIZE)) != OK)
		  panic(wName(), "couldn't read BIOS", s);
		if ((numDrives = params[0]) > 2)
		  numDrives = 2;

		for (drive = 0, wn = wini; drive < COMPAT_DRIVES; ++drive, ++wn) {
			if (drive < numDrives) {
				/* Copy te BIOS parameter vector */
				vector = (drive == 0) ? BIOS_HD0_PARAMS_ADDR : BIOS_HD1_PARAMS_ADDR;
				size = (drive == 0) ? BIOS_HD0_PARAMS_SIZE : BIOS_HD1_PARAMS_SIZE;
				if ((s = sysVirCopy(SELF, BIOS_SEG, vector, 
							SELF, D, (vir_bytes) pairVal, size)) != OK)
				  panic(wName(), "couldn't read BIOS", s);

				/* Calculate the address of the parameters and copy them */
				vector = hclickToPhys(pairVal[1]) + pairVal[0];	/* BIOS vector uses [seg:off] => seg << 4 + off */
				if ((s = sysVirCopy(SELF, BIOS_SEG, vector,
							SELF, D, (phys_bytes) params, PARAMS_LEN)) != OK)
				  panic(wName(), "couldn't copy parameters", s);
				
				/* Copy the parameters to the structures of the drive */
				wn->lCylinders = bp_cylinders(params);
				wn->lHeads = bp_heads(params);
				wn->lSectors = bp_sectors(params);
				wn->precomp = bp_precomp(params);
			}
		}
					printf("%d", size, vector, pairVal);
	}

}

int main() {
	initParams();
	return OK;
}
