#include "at_wini.h"
#include "../libpci/pci.h"
#include <minix/sysutil.h>
#include <sys/ioc_disk.h>

#define ATAPI_DEBUG			1		/* To debug ATAPI code. */

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
#define   STATUS_DRDY		0x40	/* Drive ready */
#define   STATUS_WF			0x20	/* Write fault */
#define   STATUS_DMADF		0x20	/* DMA ready / drive fault */
#define   STATUS_SRVCDSC	0x10	/* Service or dsc */
#define	  STATUS_DRQ		0x08	/* Data transfer request */
#define   STATUS_CORR		0x04	/* Correctable error occured */
#define	  STATUS_ERR		0x01	/* Error */
#define   STATUS_CHECK		0x01	/* Check error */
#define   STATUS_ADMBSY		0x100	/* Administratively busy (software) */

#define REG_ERROR			1		/* Error code */
#define   ERROR_BB			0x80	/* Bad block */
#define	  ERROR_ECC			0x40	/* Bad ecc bytes */
#define   ERROR_ID			0x10	/* Id not found */
#define	  ERROR_AC			0x04	/* Aborted command */
#define   ERROR_TK			0x02	/* Track zero error */
#define   ERROR_DM			0x01	/* No data address mark */

/* Write only register */
#define REG_COMMAND			7		/* Command */
#define   CMD_IDLE			0x00	/* For currCmd: drive idle */
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
#define ATAPI_PACKET_CMD	0xA0	/* Packet command */
#define ATAPI_IDENTIFY		0xA1	/* Identify device */
#define SCSI_READ_10		0X28	/* Read from disk */

#define REG_FEAT			1		/* Features register */
#define REG_IRR				2		/* Interrupt reason register */
#define   IRR_REL			0x04	/* Bus release */
#define   IRR_IO			0x02	/* Direction for transfer */
#define   IRR_COD			0x01	/* Command or data */
#define REG_CNT_LO			4		/* Byte count low register */
#define REG_CNT_HI			5		/* Byte count high register */

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

#define ATAPI_PACKET_SIZE	12

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
static int wTesting = 0, wSilent = 0, wLba48 = 0, wStandardTimeouts = 0, 
		   wPciDebug = 1, atapiDebug = 1;
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
static int currDevNum = -1;
static char wIdString[40];

static int currCmd;		/* Current command in execution */
static int currDrive;			/* Selected drive */
static Device * currDev;		/* Device's base and size */

static char *wName();
static Device *wPrepare(int dev);
static int wDoOpen(Driver *dp, Message *msg);
static int wDoClose(Driver *dp, Message *msg);
static int wReset();
static int wTransfer(int pNum, int opCode, off_t position, IOVec *iov, unsigned numReq);
static int atapiOpen();
static void wGeometry(Partition *entry);
static int wHwInt(Driver *dp, Message *msg);
static int wOther(Driver *dp, Message *msg);

/* Entry points to this driver. */
static Driver wDriverTable = {
	wName,		/* Current device's name */
	wDoOpen,	/* Open or mount request, initialize device */
	wDoClose,	/* Release device */
	doDrIoctl,	/* Get or set a partition's geometry */
	wPrepare,	/* Prepare for I/O on a given minor device */
	wTransfer,	/* Do the I/O */
	nopCleanup,	/* Nothing to clean up */
	wGeometry,	/* Tell the geometry of the disk */
	nopSignal,	/* No cleanup needed on shutdown */
	nopAlarm,	/* Ignore leftover alarms */
	nopCancel,	/* Ignore CANCELs */
	nopSelect,	/* Ignore selects */
	wOther,		/* Catch-all for unrecognized commands and ioctls */
	wHwInt,		/* Leftover hardware interrupts */
};

static char *wName() {
	static char name[] = "AT-D0";

	name[4] = '0' + currDrive;
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
				if (wPciDebug)
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
			if (wPciDebug)
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
	currDevNum = device;

	/* Disk drive has a partition table which has 4 partitions at most.
	 * Wini has DEV_PER_DRIVE partitions and SUB_PER_DRIVE sub-partitions at most.
	 *   DEV_PER_DRIVE = 1 + NR_PARTITIONS(4) = 5, the 1 is a logic partition for the total drive.
	 *   SUB_PER_DRIVE = NR_PARTITIONS(4) * NR_PARTITIONS(4) = 16, since each primary partition has 4 subPartitions.
	 *
	 * We assume machine has MAX_DRIVES(8) disk drives at most.
	 *   NR_MINORS = MAX_DRIVES(8) * DEV_PER_DRIVE(5) = 40
	 *   NR_SUB_DEVS = MAX_DRIVES(8) * SUB_PER_DRIVE(16) = 128
	 *   MINOR_d0p0s0 = MAX_DRIVES(8) * SUB_PER_DRIVE(16) = 128 (drive[0-7], part[0-3], sub-part[0-3])
	 *
	 * 1. When using primary partition:
	 *     device = drive * DEV_PER_DRIVE(5) + partIdx;
	 * 2. When using sub-partition:
	 *     device = MINOR_d0p0s0 + drive * SUB_PER_DRIVE(16) + partIdx * NR_PARTITIONS(4) + subPartIdx;
	 */
	if (device < NR_MINORS) {	/* d0, d0p[0-3], d1, ... */
		currDrive = device / DEV_PER_DRIVE;	/* Save drive number */
		currWn = &winiList[currDrive];
		currDev = &currWn->part[device % DEV_PER_DRIVE];
	} else if ((unsigned) (device -= MINOR_d0p0s0) < NR_SUB_DEVS) {	/* d[0-7]p[0-3]s[0-3] */
		currDrive = device / SUB_PER_DRIVE;
		currWn = &winiList[currDrive];
		currDev = &currWn->subPart[device % SUB_PER_DRIVE];
	} else {
		currDevNum = -1;
		return NIL_DEV;
	}
	return currDev;
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
	currCmd = cmd->command;
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

	switch (currCmd) {
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
			  printf("%s: timeout on command %02x\n", wName(), currCmd);
			wNeedReset();
			wn->wStatus = 0;
	}
}

static void ackIrqs(unsigned int irqs) {
	Wini *wn;
	unsigned int dr;
	for (dr = 0; dr < MAX_DRIVES && irqs; ++dr) {
		wn = &winiList[dr];
		if (!(wn->state & IGNORING) && wn->irqNeedAck && (wn->irqMask & irqs)) {
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
	currCmd = CMD_IDLE;
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
		cmd.ldh = wn->ldhPref | (wn->pHeads - 1);	/* Max head number */
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
		if ((idByte(49)[1] & 0x02) && size > 512L * 1024 * 2) {
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
		wn->irq = currDrive < 2 ? AT_WINI_0_IRQ : AT_WINI_1_IRQ;
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
	saveDev = currDevNum;

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
	if (wPrepare(currDrive * DEV_PER_DRIVE) == NIL_DEV)
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
		partition(&wDriverTable, currDrive * DEV_PER_DRIVE, P_PRIMARY);
	}
	++wn->openCount;
	return OK;
}

static void atapiClose() {
/* Shoul unlock the device. For now do nothing. */
}

static int wDoClose(Driver *dp, Message *msg) {
/*Device close: Release a device. */
	if (wPrepare(msg->DEVICE) == NIL_DEV)
	  return ENXIO;
	--currWn->openCount;
	if (currWn->openCount == 0 && (currWn->state & ATAPI))
	  atapiClose();
	return OK;
}

static int doTransfer(Wini *wn, unsigned int precomp, unsigned int count, 
			unsigned int blockAddr, unsigned int opCode) {
	Command cmd;
	unsigned sectorsPerCylinder;
	int cylinder, head, sector;

	cmd.precomp = precomp;
	cmd.count = count;
	cmd.command = opCode == DEV_SCATTER ? CMD_WRITE : CMD_READ;

	if (wn->ldhPref & LDH_LBA) {
		cmd.sector = (blockAddr >> 0) & 0xFF;
		cmd.cylinderLow = (blockAddr >> 8) & 0xFF;
		cmd.cylinderHigh = (blockAddr >> 16) & 0xFF;
		cmd.ldh = wn->ldhPref | ((blockAddr >> 24) & 0xF);
	} else {
		/* Refer to CHS. */
		sectorsPerCylinder = wn->pHeads * wn->pSectors;
		cylinder = blockAddr / sectorsPerCylinder;
		head = (blockAddr % sectorsPerCylinder) / wn->pSectors;
		sector = blockAddr % wn->pSectors;
		
		cmd.sector = sector + 1;
		cmd.cylinderLow = cylinder & BYTE;
		cmd.cylinderHigh = (cylinder >> 8) & BYTE;
		cmd.ldh = wn->ldhPref | head;
	}

	return cmdOut(&cmd);
}

static int atapiSendPacket(u8_t *packet, unsigned count) {
/* Send an ATAPI Packet Command. */
	Wini *wn = currWn;
	PvBytePair outBPs[6];		
	int s;

	if (wn->state & IGNORING)
	  return ERR;

	/* Select Master/Slave drive */
	if ((s = sysOutb(wn->baseCmd + REG_LDH, wn->ldhPref)) != OK)
	  panic(wName(), "Couldn't select master/slave drive", s);

	if (!wWaitFor(STATUS_BSY | STATUS_DRQ, 0)) {
		printf("%s: atapiSendPacket: drive not ready\n", wName());
		return ERR;
	}

	sysSetAlarm(wakeupTicks, 0);

	/* The value FFFFh is interpreted by the device as though the 
	 * value were FFFEh.
	 */
	if (count > 0xFFFE)		
	  count = 0xFFFE;		/* Max data per interrupt. */
	
	currCmd = ATAPI_PACKET_CMD;
	pvSet(outBPs[0], wn->baseCmd + REG_FEAT, 0);
	pvSet(outBPs[1], wn->baseCmd + REG_IRR, 0);
	pvSet(outBPs[2], wn->baseCmd + REG_SECTOR, 0);
	pvSet(outBPs[3], wn->baseCmd + REG_CNT_LO, (count >> 0) & 0xFF);
	pvSet(outBPs[4], wn->baseCmd + REG_CNT_HI, (count >> 8) & 0xFF);
	pvSet(outBPs[5], wn->baseCmd + REG_COMMAND, currCmd);

	if (atapiDebug)
	  printf("cmd: %x  ", currCmd);
	if ((s = sysVecOutb(outBPs, 6)) != OK)
	  panic(wName(), "Couldn't write registers with sysVecOutb()", s);
	
	if (!wWaitFor(STATUS_BSY | STATUS_DRQ, STATUS_DRQ)) {
		printf("%s: timeout (BSY|DRQ -> DRQ)\n", wName());
		return ERR;
	}
	wn->wStatus |= STATUS_ADMBSY;	/* Command not at all done yet. */

	/* Send the command packet to the device. */
	if ((s = sysOutsw(wn->baseCmd + REG_DATA, SELF, packet, ATAPI_PACKET_SIZE)) != OK)
	  panic(wName(), "sysOutsw failed", s);

	return OK;
}

#define STSTR(a)	if (status & STATUS_ ## a) { strcat(str, #a); strcat(str, " "); }
#define ERRSTR(a)	if (e & ERROR_ ## a) { strcat(str, #a); strcat(str, " "); }
static char *strStatus(int status) {
	static char str[200];
	str[0] = '\0';

	STSTR(BSY);
	STSTR(DRDY);
	STSTR(DMADF);
	STSTR(SRVCDSC);
	STSTR(DRQ);
	STSTR(CORR);
	STSTR(ERR);

	return str;
}

static char *strError(int e) {
	static char str[200];
	str[0] = '\0';

	ERRSTR(BB);
	ERRSTR(ECC);
	ERRSTR(ID);
	ERRSTR(AC);
	ERRSTR(TK);
	ERRSTR(DM);

	return str;
}

static int atapiIntrWait() {
/* Wait for an interrupt and study the results. Returns a number of bytes
 * that need to be transferred, or an error code.
 */
	Wini *wn = currWn;
	PvBytePair inBPs[4];
	int err, len, irr, s, r, phase;

	inBPs[0].port = wn->baseCmd + REG_ERROR;
	inBPs[1].port = wn->baseCmd + REG_CNT_LO;
	inBPs[2].port = wn->baseCmd + REG_CNT_HI;
	inBPs[3].port = wn->baseCmd + REG_IRR;
	if ((s = sysVecInb(inBPs, 4)) != OK)
	  panic(wName(), "ATAPI failed sysVecInb()", s);
	err = inBPs[0].value;
	len = inBPs[1].value;
	len |= inBPs[2].value << 8;
	irr = inBPs[3].value;
	
	if (ATAPI_DEBUG)
	  printf("wn %p  S=%x=%s E=%02x=%s L=%04x I=%02x\n", wn, wn->wStatus, 
			strStatus(wn->wStatus), err, strError(err), len, irr);

	if (wn->wStatus & (STATUS_BSY | STATUS_CHECK)) {
		if (atapiDebug) 
		  printf("atapi fail:  S=%x=%s E=%02x=%s L=%04x I=%02x\n", wn, wn->wStatus, 
			strStatus(wn->wStatus), err, strError(err), len, irr);
		return ERR;
	}

	phase = (wn->wStatus & STATUS_DRQ) | (irr & (IRR_COD | IRR_IO));
	
	switch (phase) {
		/* Successful command completion: DRQ=0, I/O=1, C/D=1 */
		case IRR_COD | IRR_IO:
			if (ATAPI_DEBUG)
			  printf("ACD: Phase Command Complete\n");
			r = OK;
			break;

		/* Bus release: DRQ=0, I/O=0, C/D=0 */
		case 0:
			if (ATAPI_DEBUG)
			  printf("ACD: Phase Command Aborted\n");
			r = ERR;
			break;

		/* Device awaiting command: DRQ=1, I/O=0, C/D=1 */
		case STATUS_DRQ | IRR_COD:
			if (ATAPI_DEBUG)
			  printf("ACD: Phase Command Out\n");
			r = ERR;
			break;

		/* Data transfer to device: DRQ=1, I/O=0, C/D=0 */
		case STATUS_DRQ:
			if (ATAPI_DEBUG)
			  printf("ACD: Phase Data Out %d\n", len);
			r = len;
			break;
	
		/* Data transfer to host: DRQ=1, I/O=1, C/D=0 */
		case STATUS_DRQ | IRR_IO:
			if (ATAPI_DEBUG)
			  printf("ACD: Phase Data In %d\n", len);
			r = len;
			break;

		default:
			if (ATAPI_DEBUG)
			  printf("ACD: Phase Unknown\n");
			r = ERR;
			break;
	}

	/* We need to read data after receiving this interrupt if 
	 * the data transmission is from device to host.
	 * */
	wn->wStatus |= STATUS_ADMBSY;		
	return r;
}

static int atapiTransfer(int pNum, int opCode, off_t position, 
			IOVec *iov, unsigned numReq) {
	Wini *wn = currWn;
	IOVec *iop, *iovEnd = iov + numReq;
	int r, s, errors, fresh;
	u64_t dvPos;
	unsigned long blockAddr;
	unsigned long dvSize = cv64ul(currDev->dv_size);
	unsigned numBytes, numBlocks, count, before, chunk;
	u8_t packet[ATAPI_PACKET_SIZE];

	errors = fresh = 0;

	while (numReq > 0 && !fresh) {
		/* The Minix block size is smaller than the CD block size, so we
		 * may have to read extra before or after the good data.
		 */
		dvPos = add64ul(currDev->dv_base, position);
		blockAddr = div64u(dvPos, CD_SECTOR_SIZE);
		before = rem64u(dvPos, CD_SECTOR_SIZE);

		/* How many bytes to transfer? */
		numBytes = count = 0;
		for (iop = iov; iop < iovEnd; ++iop) {
			numBytes += iop->iov_size;
			if ((before + numBytes) % CD_SECTOR_SIZE == 0)
			  count = numBytes;
		}

		/* Does one of the memory chunks end nicely on a CD sector multiple? */
		if (count != 0)
		  numBytes = count;

		/* Data comes in as words, so we have to enforce even byte counts. */
		if ((before | numBytes) & 1)
		  return EINVAL;

		/* Which block on disk and how close to EOF? */
		if (position >= dvSize)
		  return OK;	/* At EOF */
		if (position + numBytes > dvSize)
		  numBytes = dvSize - position;

		numBlocks = (before + numBytes + CD_SECTOR_SIZE - 1) / CD_SECTOR_SIZE;
		if (ATAPI_DEBUG) {
			printf("block=%lu, before=%u, numBytes=%u, numBlocks=%u\n", 
						blockAddr, before, numBytes, numBlocks);
		}

		/* First check to see if a reinitialization is needed. */
		if (!(wn->state &INITIALIZED) && wSpecify() != OK)
		  return EIO;

		/* Build an ATAPI command packet. Refer to SCSI commands. */
		packet[0] = SCSI_READ_10;
		packet[1] = 0;		/* RDPROTECT, DP0, FUA, RARC */
		packet[2] = (blockAddr >> 24) & 0xFF;	/* 2-5 is the logical block address */
		packet[3] = (blockAddr >> 16) & 0xFF;
		packet[4] = (blockAddr >> 8) & 0xFF;
		packet[5] = (blockAddr >> 0) & 0xFF;
		packet[6] = 0;		/* Reserved & Group Number */
		packet[7] = (numBlocks >> 8) & 0xFF;	/* 7-8 is the transfer length */
		packet[8] = (numBlocks >> 0) & 0xFF;
		packet[9] = 0;		/* Control */
		packet[10] = 0;
		packet[11] = 0;

		/* Tell the controller to execute the packet command. */
		if ((r = atapiSendPacket(packet, numBlocks * CD_SECTOR_SIZE)) != OK)
		  goto err;
	
		/* Read chunks of data. */
		while ((r = atapiIntrWait()) > 0) {
			count = r;

			if (ATAPI_DEBUG) 
			  printf("before=%u, numBytes=%u, count=%u\n", before, numBytes, count);

			/* Discard before. */
			while (before > 0 && count > 0) {
				chunk = before;
				if (chunk > count)
				  chunk = count;
				if (chunk > DMA_BUF_SIZE)
				  chunk = DMA_BUF_SIZE;
				if ((s = sysInsw(wn->baseCmd + REG_DATA, SELF, tmpBuf, chunk)) != OK)
				  panic(wName(), "Call to sysInsw() failed", s);
				before -= chunk;
				count -= chunk;
			}

			/* Read requested data */
			while (numBytes > 0 && count > 0) {
				chunk = numBytes;
				if (chunk > count)
				  chunk = count;
				if (chunk > iov->iov_size)
				  chunk = iov->iov_size;
				if ((s = sysInsw(wn->baseCmd + REG_DATA, pNum, 
							(void *) iov->iov_addr, chunk)) != OK)
				  panic(wName(), "Call to sysInsw() failed", s);
				position += chunk;
				numBytes -= chunk;
				count -= chunk;
				iov->iov_addr += chunk;
				fresh = 0;
				
				if ((iov->iov_size -= chunk) == 0) {
					++iov;
					--numReq;
					fresh = 1;		/* New element is optional */
				}
			}

			/* Excess data */
			while (count > 0) {
				chunk = count;
				if (chunk > DMA_BUF_SIZE)
				  chunk = DMA_BUF_SIZE;
				if ((s = sysInsw(wn->baseCmd + REG_DATA, SELF, 
							tmpBuf, chunk)) != OK)
				  panic(wName(), "Call to sysInsw() failed", s);
				count -= chunk;
			}
		}

		if (r < 0) {
err:		/* Don't retry if too many errors. */
			if (++errors == maxErrors) {
				currCmd = CMD_IDLE;
				if (atapiDebug)
				  printf("giving up (%d)\n", errors);
				return EIO;
			}
			if (atapiDebug)
			  printf("retry (%d)\n", errors);
		}
	}
	currCmd = CMD_IDLE;
	return OK;
}

static int wTransfer(int pNum, int opCode, off_t position, IOVec *iov, 
		unsigned numReq) {
	Wini *wn = currWn;
	IOVec *iop, *iovEnd = iov + numReq;
	int r, s, errors;
	unsigned long blockAddr;
	unsigned long dvSize = cv64ul(currDev->dv_size);
	unsigned numBytes;

	if (wn->state & ATAPI)
	  return atapiTransfer(pNum, opCode, position, iov, numReq);

	/* Check disk address. */
	if ((position & SECTOR_MASK) != 0)
	  return EINVAL;

	errors = 0;
	while (numReq > 0) {
		/* How many bytes to transfer? */
		numBytes = 0;
		for (iop = iov; iop < iovEnd; ++iop) {
			numBytes += iop->iov_size;
		}
		if ((numBytes & SECTOR_MASK) != 0) 
		  return EINVAL;

		/* Which block on disk and how close to EOF? */
		if (position >= dvSize)  
		  return OK;		/* At EOF */
		if (position + numBytes > dvSize)
		  numBytes = dvSize - position;

		/* Calculate sector number which may be LBA or CHS, 
		 * depends on the drive. 
		 * */
		blockAddr = div64u(add64ul(currDev->dv_base, position), 
						SECTOR_SIZE);

		if (numBytes >= wn->maxCount) {
			/* The drive can't do more than maxCount at once. */
			numBytes = wn->maxCount;
		}

		/* First check to see if a reinitialization is needed. */
		if (!(wn->state & INITIALIZED) && wSpecify() != OK)
		  return EIO;

		/* Tell the controller to transfer numBytes bytes. 
		 * The sector count register only support 256 sectors. (00h - FFh)
		 * 00h indicates 256 sectors.
		 *
		 * 'block' may be LBA or CHS, depends on the drive.
		 */
		r = doTransfer(wn, wn->precomp, (numBytes >> SECTOR_SHIFT) & BYTE, 
					blockAddr, opCode);

		while (r == OK && numBytes > 0) {
			/* For each sector, wait for an interrupt and fetch the data
			 * (read), or supply data to the controller and wait for an
			 * interrupt (write).
			 */
			if (opCode == DEV_GATHER) {
				/* First an interrupt, then data. */
				if ((r = atIntrWait()) != OK) {
					/* An error, send data to the bit bucket. */
					if (wn->wStatus & STATUS_DRQ) {
						if ((s = sysInsw(wn->baseCmd + REG_DATA, SELF, 
									tmpBuf, SECTOR_SIZE)) != OK)
						  panic(wName(), "Call to sysInsw() failed", s);
					}
					break;
				}
			}

			/* Wait for data transfer requested. */
			if (!wWaitFor(STATUS_DRQ, STATUS_DRQ)) {
				r = ERR;
				break;
			}

			/* Copy bytes to or from the device's buffer. */
			if (opCode == DEV_GATHER) {
				if ((s = sysInsw(wn->baseCmd + REG_DATA, pNum, 
							(void *) iov->iov_addr, SECTOR_SIZE)) != OK) 
				  panic(wName(), "Call to sysInsw() failed", s);
			} else {
				if ((s = sysOutsw(wn->baseCmd + REG_DATA, pNum,
							(void *) iov->iov_addr, SECTOR_SIZE)) != OK)
				  panic(wName(), "Call to sysOutsw() failed", s);

				/* Data sent, wait for an interrupt. */
				if ((r = atIntrWait()) != OK)
				  break;
			}

			/* Book the bytes successfully transferred. */
			numBytes -= SECTOR_SIZE;
			position += SECTOR_SIZE;
			iov->iov_addr += SECTOR_SIZE;
			if ((iov->iov_size -= SECTOR_SIZE) == 0) {
				++iov;
				--numReq;
			}
		}

		/* Any errors? */
		if (r != OK) {
			/* Don't retry if sector marked bad or too many errors. */
			if (r == ERR_BAD_SECTOR || ++errors == maxErrors) {
				currCmd = CMD_IDLE;
				return EIO;
			}
		}
	}

	currCmd = CMD_IDLE;
	return OK;
}

static void wGeometry(Partition *entry) {
	Wini *wn = currWn;

	if (wn->state & ATAPI) {	/* Make up some numbers. */
		entry->cylinders = div64u(wn->part[0].dv_size, SECTOR_SIZE) / (64 * 32);
		entry->heads = 64;
		entry->sectors = 32;
	} else {	/* Return logical geometry */
		entry->cylinders = wn->lCylinders;
		entry->heads = wn->lHeads;
		entry->sectors = wn->lSectors;
	}
}

static int wHwInt(Driver *dp, Message *msg) {
/* Leftover interrupt(s) received; ack it/them. */
	ackIrqs(msg->NOTIFY_ARG);
	return OK;
}

static int wOther(Driver *dp, Message *msg) {
	int r, timeout, prev, count;

	if (msg->m_type != DEV_IOCTL) 
	  return EINVAL;

	if (msg->REQUEST == DIOC_TIMEOUT) {
		if ((r = sysDataCopy(msg->PROC_NR, (vir_bytes) msg->ADDRESS, 
					SELF, (vir_bytes) &timeout, sizeof(timeout))) != OK)
		  return r;

		if (timeout == 0) {
			/* Restore defaults. */
			timeoutTicks = DEF_TIMEOUT_TICKS;
			maxErrors = MAX_ERRORS;
			wakeupTicks = WAKEUP;
			wSilent = 0;
		} else if (timeout < 0) {
			return EINVAL;
		} else {
			prev = wakeupTicks;

			if (!wStandardTimeouts) {
				/* Set (lower) timeout, lower error tolerance and 
				 * set silent mode.
				 */
				wakeupTicks = timeout;
				maxErrors = 3;
				wSilent = 1;

				if (timeoutTicks > timeout)
				  timeoutTicks = timeout;
			}

			if ((r = sysDataCopy(SELF, (vir_bytes) &prev,
						msg->PROC_NR, (vir_bytes) msg->ADDRESS, sizeof(prev))) != OK)
			  return r;
		}
		return OK;
	} else if (msg->REQUEST == DIOC_OPEN_COUNT) {
		if (wPrepare(msg->DEVICE) == NIL_DEV)
		  return ENXIO;
		count = currWn->openCount;
		if ((r = sysDataCopy(SELF, (vir_bytes) &count,
					msg->PROC_NR, (vir_bytes) msg->ADDRESS, sizeof(count))) != OK)
		  return r;
		return OK;
	}
	return EINVAL;
}

int main() {
	initParams();
	driverTask(&wDriverTable);
	return OK;
}


