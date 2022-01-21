#include "at_wini.h"
#include "../libpci/pci.h"

#include "minix/sysutil.h"

/* I/O Ports used by winchester disk controllers. */

/* Read and write registers */
#define REG_CMD_BASE0		0x1F0	/* Command base register of controller 0 */
#define REG_CMD_BASE1		0x170	/* Command base register of controller 1 */
#define REG_CTL_BASE0		0x3F6	/* Control base register of controller 0 */
#define REG_CTL_BASE1		0x376	/* Control base register of controller 1 */

#define LDH_DEFAULT			0xA0	/* ECC enable, 512 bytes per sector */
#define LDH_LBA				0x40	/* Use LBA addressing */
#define ldhInit(drive)		(LDH_DEFAULT | ((drive) << 4))


#define MAX_SECTORS			256		/* Controller can transfer this many sectors */
#define MAX_DRIVES			8
#define COMPAT_DRIVES		4

/* Interrupt request lines. */
#define NO_IRQ				0		/* No IRQ set yet */

static long wInstance = 0;
static int wNextDrive = 0;

typedef struct {		/* Main drive struct, one entry per drive */
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
	unsigned ldhPref;		/* Top four bytes of the LDH (head) register */
	unsigned precomp;		/* Write precompensation cylinder / 4 */
	unsigned maxCount;		/* Max request for this drive */
	unsigned openCount;		/* In-use count */
} Wini;
static Wini wini[MAX_DRIVES];

static int wDrive;				/* Selected drive */

static char *wName() {
	static char name[] = "AT-D0";

	name[4] = '0' + wDrive;
	return name;
}

static void initDrive(Wini *w, int baseCmd, int baseCtl, int irq, int ack, int hook, int drive) {
	w->state = 0;
	w->wStatus = 0;
	w->baseCmdReg = baseCmd;
	w->baseCtlReg = baseCtl;
	w->irq = irq;
	w->irqMask = 1 << irq;
	w->irqNeedAck = ack;
	w->irqHookId = hook;
	w->ldhPref = ldhInit(drive);
	w->maxCount = MAX_SECTORS << SECTOR_SHIFT;
	w->lba48 = 0;
}

static void initPciParams(int skip) {
	initPci();
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

			/* Fill in non-BIOS parameters. */
			initDrive(wn, 
				drive < 2 ? REG_CMD_BASE0 : REG_CMD_BASE1,
				drive < 2 ? REG_CTL_BASE0 : REG_CTL_BASE1,
				NO_IRQ, 0, 0, drive);
			++wNextDrive;
		}
	}
	
	/* Look for controllers on the pci bus. Skip none the first instance,
	 * skip one and then 2 for every instance, for every next instance.
	 */
	if (wInstance == 0)
	  initPciParams(0);
	else
	  initPciParams(wInstance * 2 - 1);
}

int main() {
	initParams();
	return OK;
}
