#include "at_wini.h"
#include "../libpci/pci.h"

#include "minix/sysutil.h"

/* I/O Ports used by winchester disk controllers. */

/* Read and write registers */
#define REG_CMD_BASE0		0x1F0	/* Command base register of controller 0 */
#define REG_CMD_BASE1		0x170	/* Command base register of controller 1 */
#define REG_CTL_BASE0		0x3F6	/* Control base register of controller 0 */
#define REG_CTL_BASE1		0x376	/* Control base register of controller 1 */

#define	REG_DATA			0		/* Data register (offset from the base reg.) */
#define REG_PRECOMP			1		/* Start of write precompensation */
#define REG_COUNT			2		/* Sectors to transfer */
#define REG_SECTOR			3		/* Sector number */
#define REG_CYL_LO			4		/* Low byte of cylinder number */
#define REG_CYL_HI			5		/* High byte of cylinder number */
#define REG_LDH				6		/* LBA, drive and head */
#define   LDH_DEFAULT		0xA0	/* ECC enable, 512 bytes per sector */
#define   LDH_LBA			0x40	/* Use LBA addressing */
#define   ldhInit(drive)	(LDH_DEFAULT | ((drive) << 4))

/* Read only registers */
#define REG_STATUS			7		/* Status */
#define   STATUS_BSY		0x80	/* Controller busy */
#define   STATUS_RDY		0x40	/* Drive ready */
#define   STATUS_WF			0x20	/* Write fault */
#define	  STATUS_ERR		0x01	/* Error */
#define   STATUS_ADMBSY		0x100	/* Administratively busy (software) */
#define REG_ERROR			1		/* Error code */
#define   ERROR_BB			0x80	/* Bad block */

/* Write only register */
#define REG_COMMAND			7		/* Command */
#define   CMD_IDLE			0x00	/* For wCurrCmd: drive idle */
#define	  CMD_RECALIBRATE	0x10	/* Recalibrate drive */
#define   CMD_READ			0X20	/* Read data */
#define   CMD_WRITE			0x30	/* Write data */
#define	  CMD_SPECIFY		0x91	/* Specify parameters */
#define   ATA_IDENTIFY		0xEC	/* Identify drive */

/* Control register */
#define REG_CTL				0		/* Control register */
#define   CTL_NO_RETRY		0x80	/* Disable access retry */
#define	  CTL_NO_ECC		0x40	/* Disable ecc retry */
#define   CTL_EIGHT_HEADS	0x08	/* More than eight heads */
#define   CTL_RESET			0x04	/* Reset controller */
#define	  CTL_INT_DISABLE	0x02	/* Disable interrupts */

/* ATAPI */
#define ATAPI_IDENTIFY		0xA1	/* Identify device */

#define CD_SECTOR_SIZE		2048	/* Sector size of a CD-ROM */

#define MAX_SECTORS			256		/* Controller can transfer this many sectors */
#define MAX_DRIVES			8
#define COMPAT_DRIVES		4

#define MAX_ERRORS			4		/* How often to try R/W before quitting */
#define NR_MINORS			(MAX_DRIVES * DEV_PER_DRIVE)
#define DELAY_TICKS			1		/* Controller timeout in ticks */
#define DEF_TIMEOUT_TICKS	300		/* Controller timeout in ticks */
#define RECOVERY_TICKS		30		/* Controller recovery time in ticks */
#define INITIALIZED			0x01	/* Drive is initialized */
#define	DEAF				0x02	/* Controller must be reset */
#define SMART				0x04	/* Drive supports ATA commands */
#define ATAPI				0x08	/* It is an ATAPI device */
#define IDENTIFIED			0x10	/* wIdentify done successfully */
#define IGNORING			0x20	/* wIdentify failed once */

#define ATA_IF_NOT_COMPAT1	(1L << 0)
#define ATA_IF_NOT_COMPAT2	(1L << 2)

/* Interrupt request lines. */
#define NO_IRQ				0		/* No IRQ set yet */

#define SUB_PER_DRIVE	(NR_PARTITIONS * NR_PARTITIONS)
#define NR_SUB_DEVS		(MAX_DRIVES * SUB_PER_DRIVE)

/* Error codes */
#define	ERR			(-1)	/* General error */
#define ERR_BAD_SECTOR	(-2)	/* Block marked bad detected */

/* Some controllers don't interrupt, the clock will wake us up. */
#define WAKEUP		(32 * HZ)	/* Drive may be out for 31 seconds max */

/* Timeouts and max retries. */
static int timeoutTicks = DEF_TIMEOUT_TICKS, maxErrors = MAX_ERRORS;
static int wakeupTicks = WAKEUP;
static int wTesting = 0, wSilent = 0, wLba48 = 0, wStandardTimeouts = 0;
static int wInstance = 0;
static int wNextDrive = 0;

/* Common command block */
typedef struct {
	u8_t precomp;		/* REG_PRECOMP, etc. */
	u8_t count;
	u8_t sector;
	u8_t cylinderLow;
	u8_t cylinderHigh;
	u8_t ldh;
	u8_t command;
} Command;

typedef struct {		/* Main drive struct, one entry per drive */
	unsigned state;			/* Drive state: deaf, initialized, dead */
	unsigned wStatus;		/* Device status register */
	unsigned baseCmd;	/* Command base register */
	unsigned baseCtl;	/* Control base register */
	unsigned irq;			/* Interrupt request line */
	unsigned irqMask;		/* 1 << irq */
	unsigned irqNeedAck;	/* irq needs to be acknowledged */
	int irqHookId;			/* Id of IrqHook at the kernel */
	int lba48;				/* Supports lba48 */
	unsigned lCylinders;	/* Logical number of cylinders (BIOS) */
	unsigned lHeads;		/* Logical number of heads */
	unsigned lSectors;		/* Logical number of sectors per track */
	unsigned pCylinders;	/* Physical number of cylinders (translated) */
	unsigned pHeads;		/* Physical number of heads */
	unsigned pSectors;		/* Physical number of sectors per track */
	unsigned ldhPref;		/* Top four bytes of the LDH (head) register */
	unsigned precomp;		/* Write precompensation cylinder / 4 */
	unsigned maxCount;		/* Max request for this drive */
	unsigned openCount;		/* In-use count */
	Device part[DEV_PER_DRIVE];		/* Disks and partitions */
	Device subPart[SUB_PER_DRIVE];	/* Sub partitions */
} Wini;
static Wini winiList[MAX_DRIVES], *currWn;
static int wDevice = -1;
//static int wController = -1;
//static int wMajor = -1;
static char wIdString[40];

static int wCurrCmd;		/* Current command in execution */
static int wDrive;			/* Selected drive */
static Device * wDev;		/* Device's base and size */
static int wDrive;			/* Selected drive */

static char *wName();
static int wDoOpen(Driver *dp, Message *msg);
static int wReset();
static int wTransfer(int pNum, int opCode, off_t position, IOVec *iov, unsigned numReq);
static int atapiOpen();

/* Entry points to this driver. */
static Driver wDriver = {
	wName,		/* Current device's name */
	wDoOpen,	/* Open or mount request, initialize device */
};

static char *wName() {
	static char name[] = "AT-D0";

	name[4] = '0' + wDrive;
	return name;
}

static void initDrive(Wini *w, int baseCmd, int baseCtl, int irq, int ack, int hook, int drive) {
	w->state = 0;
	w->wStatus = 0;
	w->baseCmd = baseCmd;
	w->baseCtl = baseCtl;
	w->irq = irq;
	w->irqMask = 1 << irq;
	w->irqNeedAck = ack;
	w->irqHookId = hook;
	w->ldhPref = ldhInit(drive);
	w->maxCount = MAX_SECTORS << SECTOR_SHIFT;
	w->lba48 = 0;
}

static void initPciParams(int skip) {
	int r, drive, devInd, s;
	u16_t vid, did;
	int interface, irq, irqHook;

	initPci();

	for (drive = wNextDrive; drive < MAX_DRIVES; ++drive) {
		winiList[drive].state = IGNORING;
	}
	for (r = pciFirstAvailDev(&devInd, &vid, &did);
		r != 0 && wNextDrive < MAX_DRIVES; 
		r = pciNextAvailDev(&devInd, &vid, &did)) {
		if (pciAttrDevR8(devInd, PCI_BASE_CLASS) != 0x01 ||
			pciAttrDevR8(devInd, PCI_SUB_CLASS) != 0x01) {
		/* Base class must be 0x01 (mass storage), sub class 
		 * must be 0x01 (ATA). 
		 */
			continue;
		}
		/* Found a controller.
		 * Programming interface register tells us more.
		 */
		interface = pciAttrDevR8(devInd, PCI_PROG_IF);
		irq = pciAttrDevR8(devInd, PCI_INTR_LINE);

		/* Any non-compat drives? */
		if (interface & (ATA_IF_NOT_COMPAT1 | ATA_IF_NOT_COMPAT2)) {
			irqHook = irq;
			if (skip > 0) {
				printf("atapci skipping controller (remain %d)\n", skip);
				--skip;
				continue;
			}
			if ((s = sysIrqSetPolicy(irq, 0, &irqHook)) != OK) {
				printf("atapci: couldn't set IRQ policy %d\n", irq);
				continue;
			}
			if ((s = sysIrqEnable(&irqHook)) != OK) {
				printf("atapci: couldn't enable IRQ line %d\n", irq);
				continue;
			}
		} else {
			/* If not.. this is not the ata-pci controller we're
			 * looking for.
			 */
			printf("atapci skipping compatability controller\n");
			continue;
		}

		/* Primary channel not in compatability mode? */
		if (interface & ATA_IF_NOT_COMPAT1) {
			// TODO
		}

		/* Secondary channel not in compatability mode? */
		if (interface & ATA_IF_NOT_COMPAT2) {
			// TODO
		}
		wNextDrive += 4;
	}
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

		for (drive = 0, wn = winiList; drive < COMPAT_DRIVES; ++drive, ++wn) {
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

static Device *wPrepare(int device) {
/* Prepare for I/O on a device. */
	wDevice = device;

	if (device < NR_MINORS) {	/* d0, d0p[0-3], d1, ... */
		wDrive = device / DEV_PER_DRIVE;	/* Save drive number */
		currWn = &winiList[wDrive];
		wDev = &currWn->part[device % DEV_PER_DRIVE];
	} else if ((unsigned) (device -= MINOR_d0p0s0) < NR_SUB_DEVS) {	/* d[0-7]p[0-3]s[0-3] */
		wDrive = device / SUB_PER_DRIVE;
		currWn = &winiList[wDrive];
		wDev = &currWn->subPart[device % SUB_PER_DRIVE];
	} else {
		wDevice = -1;
		return NIL_DEV;
	}
	return wDev;
}

static void wNeedReset() {
/* The controller needs to be reset. */
	Wini *wn;
	int dr = 0;

	for (wn = winiList; wn < &winiList[MAX_DRIVES]; ++wn, ++dr) {
		if (wn->baseCmd == currWn->baseCmd) {
			wn->state |= DEAF;
			wn->state &= ~IDENTIFIED;
		}
	}
}

static int wWaitFor(int mask, int value) {
/* Wait until controller is in the required state. Return zero on timeout.
 * An alarm that set a timeout flag is used. TIMEOUT is in micros, we need
 * ticks. Disabling the alarm is not needed, because a static flag is used
 * and a leftover timeout cannot do any harm.
 */
	clock_t t0, t1;
	int s;
	getUptime(&t0);
	do {
		if ((s = sysInb(currWn->baseCmd + REG_STATUS, &currWn->wStatus)) != OK)
		  panic(wName(), "Couldn't read register", s);
		if ((currWn->wStatus & mask) == value)
		  return 1;
	} while ((s = getUptime(&t1)) == OK && (t1 - t0) < timeoutTicks);

	if (s != OK)
	  printf("AT_WINI: warning, getUptime failed: %d\n", s);

	wNeedReset();	/* Controller gone deaf */
	return 0;
}

static int cmdOut(Command *cmd) {
/* Output the command block to the winchester controller and return status. */

	Wini *wn = currWn;
	unsigned baseCmd = wn->baseCmd;
	unsigned baseCtl = wn->baseCtl;
	PvBytePair outBPs[7];
	int s;

	if (wn->state & IGNORING)
	  return ERR;

	if (!wWaitFor(STATUS_BSY, 0)) {
		printf("%s: controller not ready\n", wName());
		return ERR;
	}

	/* Select drive. */
	if ((s = sysOutb(baseCmd + REG_LDH, cmd->ldh)) != OK)
	  panic(wName(), "Couldn't write register to select drive", s);

	if (!wWaitFor(STATUS_BSY, 0)) {
		printf("%s: cmdOut: drive not ready\n", wName());
		return ERR;
	}

	/* Schedule a wakeup call, some controllers are flaky. This is done with
	 * a synchronous alarm. if a timeout occurs a SYN_ALARM message is sent
	 * from HARDWARE, so that wIntrWait() can call wTimeout() in case the
	 * controller was not able to execute the command. Leftover timeouts are
	 * simply ignored by the main loop.
	 */
	sysSetAlarm(wakeupTicks, 0);

	wn->wStatus = STATUS_ADMBSY;
	wCurrCmd = cmd->command;
	pvSet(outBPs[0], baseCtl + REG_CTL, wn->pHeads >= 8 ? CTL_EIGHT_HEADS : 0);
	pvSet(outBPs[1], baseCmd + REG_PRECOMP, cmd->precomp);
	pvSet(outBPs[2], baseCmd + REG_COUNT, cmd->count);
	pvSet(outBPs[3], baseCmd + REG_SECTOR, cmd->sector);
	pvSet(outBPs[4], baseCmd + REG_CYL_LO, cmd->cylinderLow);
	pvSet(outBPs[5], baseCmd + REG_CYL_HI, cmd->cylinderHigh);
	pvSet(outBPs[6], baseCmd + REG_COMMAND, cmd->command);

	if ((s = sysVecOutb(outBPs, 7)) != OK)
	  panic(wName(), "Couldn't write registers wit sysVOutb()", s);
	return OK;
}

static void wTimeout() {
	Wini *wn = currWn;

	switch (wCurrCmd) {
		case CMD_IDLE:
			break;	/* fine */
		case CMD_READ:
		case CMD_WRITE:
			/* Impossible, but not on PC's: The controller does not respond. */

			/* Limiting multi-sector I/O seems to help. */
			if (wn->maxCount > 8 * SECTOR_SIZE) {
				wn->maxCount = 8 * SECTOR_SIZE;
			} else {
				wn->maxCount = SECTOR_SIZE;
			}
			/* Full through */
		default:
			/* Some other commands. */
			if (wTesting)
			  wn->state |= IGNORING;	/* Kick out this drive. */
			else if (!wSilent)
			  printf("%s: timeout on command %02x\n", wName(), wCurrCmd);
			wNeedReset();
			wn->wStatus = 0;
	}
}

static void ackIrqs(unsigned int irqs) {
	Wini *wn;
	unsigned int dr;
	for (dr = 0; dr < MAX_DRIVES && irqs; ++dr) {
		wn = &winiList[dr];
		if (!(wn->state & IGNORING) && wn->irqNeedAck &&
				(wn->irqMask & irqs)) {
			if (sysInb(wn->baseCmd + REG_STATUS, &wn->wStatus) != OK)
			  printf("couldn't ack irq an drive %d\n", dr);
			if (sysIrqEnable(&wn->irqHookId) != OK)
			  printf("couldn't re-enable drive %d\n", dr);
			irqs &= ~wn->irqMask;
		}
	}
}

static void wIntrWait() {
/* Wait for a task completion interrupt. */
	Message msg;

	if (currWn->irq != NO_IRQ) {
		/* Wait for an interrupt that sets wStatus to "not busy". */
		while (currWn->wStatus & (STATUS_ADMBSY | STATUS_BSY)) {
			receive(ANY, &msg);		/* Expect HARD_INT message */
			if (msg.m_type == SYN_ALARM) {	/* But check for timeout */
				wTimeout();		/* set wStatus */
			} else if (msg.m_type == HARD_INT) {
				sysInb(currWn->baseCmd + REG_STATUS, &currWn->wStatus);
				ackIrqs(msg.NOTIFY_ARG);
			} else {
				printf("AT_WINI got unexpected message %d from %d\n", 
							msg.m_type, msg.m_source);
			}
		}
	} else {
		/* Interrupt not yet allocated; use polling. */
		wWaitFor(STATUS_BSY, 0);
	}
}

static int atIntrWait() {
/* Wait for an interrupt, study the status bits and return error/success. */
	int r;
	int s, val;

	wIntrWait();
	if ((currWn->wStatus & (STATUS_BSY | STATUS_WF | STATUS_ERR)) == 0) {
		r = OK;
	} else {
		if ((s = sysInb(currWn->baseCmd + REG_ERROR, &val)) != OK)
		  panic(wName(), "Couldn't read register", s);
		if ((currWn->wStatus & STATUS_ERR) && (val & ERROR_BB)) 
		  r = ERR_BAD_SECTOR;	/* Sector marked bad, retries won't help */
		else
		  r = ERR;		/* Any other error */
	}
	currWn->wStatus |= STATUS_ADMBSY;	/* Assume still busy with I/O */
	return r;
}

static int cmdSimple(Command *cmd) {
/* A simple controller command, only one interrupt and no data-out phase. */
	int r;

	if (currWn->state & IGNORING)
	  return ERR;

	if ((r = cmdOut(cmd)) == OK)
	  r = atIntrWait();
	wCurrCmd = CMD_IDLE;
	return r;
}

static int wSpecify() {
/* Routine to initialize the drive after boot or when a reset is needed. */
	Wini *wn = currWn;
	Command cmd;

	if ((wn->state & DEAF) && wReset() != OK)
	  return ERR;

	if (!(wn->state & ATAPI)) {
		/* Specify parameters: precompensation, number of heads and sectors. */
		cmd.precomp = wn->precomp;
		cmd.count = wn->pSectors;
		cmd.ldh = wn->ldhPref | (wn->pHeads - 1);
		cmd.command = CMD_SPECIFY;

		/* Output command block and see if controller accepts the parameters. */
		if (cmdSimple(&cmd) != OK)
		  return ERR;

		if (!(wn->state & SMART)) {
			/* Calibrate an old disk. */
			cmd.sector = 0;
			cmd.cylinderLow = 0;
			cmd.cylinderHigh = 0;
			cmd.ldh = wn->ldhPref;
			cmd.command = CMD_RECALIBRATE;

			if (cmdSimple(&cmd) != OK)
			  return ERR;
		}
	}
	wn->state |= INITIALIZED;
	return OK;
}

static int wIdentify() {
/* Find out a device exists, if it is an old AT disk, or a newer ATA
 * drive, a removable media device, etc.
 */
	Wini *wn = currWn;
	Command cmd;
	int i, s;
	unsigned long size;
#define idByte(n)	(&tmpBuf[2 * (n)])
#define idWord(n)	(((u16_t) idByte(n)[0] << 0) | \
					 ((u16_t) idByte(n)[1] << 8))
#define	idLong(n)	(((u32_t) idByte(n)[0] << 0) | \
					 ((u32_t) idByte(n)[1] << 8) | \
					 ((u32_t) idByte(n)[2] << 16) | \
					 ((u32_t) idByte(n)[3] << 24))

	/* Try to identify the device. */
	cmd.ldh = wn->ldhPref;
	cmd.command = ATA_IDENTIFY;
	if (cmdSimple(&cmd) == OK) {
		/* This is an ATA device. */
		wn->state |= SMART;

		/* Device information. */
		if ((s = sysInsw(wn->baseCmd + REG_DATA, SELF, tmpBuf, SECTOR_SIZE)) != OK)
		  panic(wName(), "Call to sysInsw() failed", s);

		/* Model Number. Why are the strings byte swapped??? */
		for (i = 0; i < 40; ++i) {
			wIdString[i] = idByte(27)[i^1];			
		}

		/* Preferred CHS translation mode. */
		wn->pCylinders = idWord(1);
		wn->pHeads = idWord(3);
		wn->pSectors = idWord(6);
		size = (u32_t) wn->pCylinders * wn->pHeads * wn->pSectors;

		/* Capacities: LBA Supported? */
		if ((idByte(49)[1] & 0x02) && size > 512L * 12024 * 2) {
			/* Drive is LBA capable and is big enough to trust it to
			 * not make a mess of it.
			 */
			wn->ldhPref |= LDH_LBA;
			size = idLong(60);	/* Total number of user addressable sectors (LBA mode only) */

			if (wLba48 && ((idWord(83)) & (1 << 10))) {
				/* Drive is LBA48 capable (and LBA48 is turned on). 
				 *
				 * 100: LBA_LSB
				 * 101: LBA_MID
				 * 102: LBA_48_MSB
				 * 103: LBA_64_MSB (reserved in future)
				 */
				if (idWord(102) || idWord(103)) {
					/* if number of sectors doesn't fit in 32 bits, truncate to this. 
					 * So it's LBA32 for now. This can still address devices up to 
					 * 2TB (4 GB * 512 bytes) though.
					 */
					size = ULONG_MAX;
				} else {
					/* Actual number of sectors fits in 32 bits.
					 * Read 100(LBA_LSB) and 101(LBA_MID).
					 */
					size = idLong(100);
				}
				wn->lba48 = 1;
			}
		}
		
		if (wn->lCylinders == 0) {
			/* No BIOS parameters? Then make some up. */
			wn->lCylinders = wn->pCylinders;
			wn->lHeads = wn->pHeads;
			wn->lSectors = wn->pSectors;
			while (wn->lCylinders > 1024) {
				wn->lHeads *= 2;
				wn->lCylinders /= 2;
			}
		}
	} else if (cmd.command = ATAPI_IDENTIFY, cmdSimple(&cmd) == OK) {
		/* An ATAPI device. */
		wn->state |= ATAPI;

		/* Device information */
		if ((s = sysInsw(wn->baseCmd + REG_DATA, SELF, tmpBuf, SECTOR_SIZE)) != OK)
		  panic(wName(), "Call to sysInsw() failed", s);

		/* Model Number. Why are the strings byte swapped??? */
		for (i = 0; i < 40; ++i) {
			wIdString[i] = idByte(27)[i^1];			
		}
		
		size = 0;	/* Size set later. */
	} else {
		/* Not an ATA device; no translation, no special features. 
		 * Don't touch it unless the BIOS knows about it. 
		 */
		if (wn->lCylinders == 0)
		  return ERR;	/* No BIOS parameters */
		wn->pCylinders = wn->lCylinders;
		wn->pHeads = wn->lHeads;
		wn->pSectors = wn->lSectors;
		size = (u32_t) wn->pCylinders * wn->pHeads * wn->pSectors;
	}

	/* Size of the whole drive. */
	wn->part[0].dv_size = mul64u(size, SECTOR_SIZE);

	/* Reset/calibrate (where necessary) */
	if (wSpecify() != OK && wSpecify() != OK) 
	  return ERR;

	if (wn->irq == NO_IRQ) {
		/* Everything looks OK; register IRQ so we can stop polling. */
		wn->irq = wDrive < 2 ? AT_WINI_0_IRQ : AT_WINI_1_IRQ;
		wn->irqHookId = wn->irq;	/* Id to be returned if interrupt occurs */
		if ((s = sysIrqSetPolicy(wn->irq, IRQ_REENABLE, &wn->irqHookId)) != OK) 
		  panic(wName(), "couldn't set IRQ policy", s);
		if ((s = sysIrqEnable(&wn->irqHookId)) != OK)
		  panic(wName(), "couldn't enable IRQ line", s);
	}
	wn->state |= IDENTIFIED;
	return OK;
}

static int wReset() {
/* Issue a reset to the controller. This is done after any catastrophe,
 * like the controller refusing to respond.
 */
	int s;
	Wini *wn = currWn;

	/* Don't bother if this drive is forgotten. */
	if (wn->state & IGNORING) 
	  return ERR;
	
	/* Wait for any internal drive recovery. */
	tickDelay(RECOVERY_TICKS);

	/* Stroke reset bit */
	if ((s = sysOutb(wn->baseCmd + REG_CTL, CTL_RESET)) != OK)
	  panic(wName(), "Couldn't stroke reset bit", s);
	tickDelay(DELAY_TICKS);
	if ((s = sysOutb(wn->baseCmd + REG_CTL, 0)) != OK)
	  panic(wName(), "Couldn't stroke reset bit", s);
	tickDelay(DELAY_TICKS);

	/* Wait for controller ready */
	if (!wWaitFor(STATUS_BSY, 0)) {
		printf("%s: reset failed, drive busy\n", wName());
		return ERR;
	}

	/* The error register should be checked now, but some drives mess it up. */
	for (wn = winiList; wn < &winiList[MAX_DRIVES]; ++wn) {
		if (wn->baseCmd == currWn->baseCmd) {
			wn->state &= ~DEAF;
			if (wn->irqNeedAck) {
				/* Make sure irq is actually enabled. */
				sysIrqEnable(&wn->irqHookId);
			}
		}
	}

	return OK;
}

static int wTestIO() {
	int r, saveDev, saveTimeout, saveErrors, saveWakeup;
	static char buf[CD_SECTOR_SIZE];
	IOVec iov;

	iov.iov_addr = (vir_bytes) buf;
	iov.iov_size = sizeof(buf);
	saveDev = wDevice;

	/* Reduce timeout values for this test transaction. */
	saveTimeout = timeoutTicks;
	saveErrors = maxErrors;
	saveWakeup = wakeupTicks;

	if (!wStandardTimeouts) {
		timeoutTicks = HZ * 4;
		wakeupTicks = HZ * 6;
		maxErrors = 3;
	}
	wTesting = 1;

	/* Try I/O on the actual drive (not any (sub)partition). */
	if (wPrepare(wDrive * DEV_PER_DRIVE) == NIL_DEV)
	  panic(wName(), "Couldn't switch devices", NO_NUM);

	r = wTransfer(SELF, DEV_GATHER, 0, &iov, 1);

	/* Switch back. */
	if (wPrepare(saveDev) == NIL_DEV)
	  panic(wName(), "Couldn't switch back devices", NO_NUM);

	/* Restore parameters. */
	timeoutTicks = saveTimeout;
	maxErrors = saveErrors;
	wakeupTicks = saveWakeup;
	wTesting = 0;

	/* Test if everything worked. */
	if (r != OK || iov.iov_size != 0)
	  return ERR;

	/* Everything worked. */
	return OK;
}

static int atapiOpen() {
/* Should load and lock the device and obtain its size. For now just set te
 * size of the device to something big. What is really needed is a generic
 * SCSI layer that does all this stuff for ATAPI and SCSI devices.
 */
	currWn->part[0].dv_size = mul64u(800L * 1024, 1024);
	return OK;
}

static int wDoOpen(Driver *dp, Message *msg) {
/* Device open: Initialize the controller and read the partition table. */
	int r;
	Wini *wn;

	if (wPrepare(msg->DEVICE) == NIL_DEV)
	  return ENXIO;

	wn = currWn;

	/* If we've probed it before and it failed, don't probe it again. */
	if (wn->state & IGNORING)
	  return ENXIO;

	/* If we haven't identified it yet, or it's gone deaf,
	 * (re-)identify it.
	 */
	if (!(wn->state & IDENTIFIED) || (wn->state & DEAF)) {
		/* Try to identify the device. */
		if (wIdentify() != OK) {
			printf("%s: probe failed\n", wName());
			if (wn->state & DEAF)
			  wReset();
			wn->state = IGNORING;
			return ENXIO;
		}
		/* Do a test transaction unless it's a CD drive (then
		 * we can believe the controller, and a test may fail
		 * due to no CD being in the drive). If it fails, ignore
		 * the device forever.
		 */
		if (!(wn->state & ATAPI) && wTestIO() != OK) {
			wn->state |= IGNORING;
			return ENXIO;
		}

		printf("%s: AT driver detected ", wName());
		if (wn->state & (SMART | ATAPI)) 
		  printf("%.40s\n", wIdString);
		else
		  printf("%ux%ux%u\n", wn->pCylinders, wn->pHeads,wn->pSectors);
	}

	if ((wn->state & ATAPI) && (msg->COUNT & W_BIT))
	  return EACCES;

	/* If it's not an ATAPI device, then don't open with RO_BIT. */
	if (!(wn->state & ATAPI) && (msg->COUNT & RO_BIT))
	  return EACCES;

	/* Partition the drive if it's being opened for the first time,
	 * or being opened after being closed.
	 */
	if (wn->openCount == 0) {
		if (wn->state & ATAPI) {
			if ((r = atapiOpen()) != OK)
			  return r;
		}
		/* Partition the disk. */
		partition(&wDriver, wDrive * DEV_PER_DRIVE, P_PRIMARY, wn->state & ATAPI);
	}
	++wn->openCount;
	return OK;
}

static int wTransfer(int pNum, int opCode, off_t position, IOVec *iov, unsigned numReq) {
	// TODO
	return OK;
}

int main() {
	initParams();
	driverTask(&wDriver);
	return OK;
}
