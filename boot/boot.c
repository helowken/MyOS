#include "code.h"
#include "stdarg.h"
#include "util.h"
#include "boot.h"
#include "partition.h"

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))

/* BIOS INT 13h errors */
static char *biosDiskError(int err) {
	static struct errEntry {
		int err;
		char *what;
	} errList[] = {
		{ 0x00, "No error" },
		{ 0x01, "Invalid command" },
		{ 0x02, "Address mark not found" },
		{ 0x03, "Disk write protected (floppy)" },
		{ 0x04, "Request sector not found" },
		{ 0x05, "Reset failed (hard disk)" },
		{ 0x06, "Floppy disk removed/Disk changeline (floppy)" },
		{ 0x07, "Bad parameter table (hard disk)/Initialization failed" },
		{ 0x08, "DMA overrun (floppy)" },
		{ 0x09, "DMA crossed 64K boundary" },
		{ 0x0A, "Bas sector flag (hard disk)" },
		{ 0x0B, "Bad track flag (hard disk)" },
		{ 0x0C, "Media type not found (floppy)" },
		{ 0x0D, "Invalid number of sectors on format (hard disk)" },
		{ 0x0E, "Control data address mark detected (hard disk)" },
		{ 0x0F, "DMA arbitration level out of range (hard error - retry failed)" },
		{ 0x10, "Uncorrectable CRC or ECC data error (hard error - retry failed)" },
		{ 0x11, "ECC corrected data error (soft error - retried OK ) (hard disk)" },
		{ 0x20, "Controller failure" },
		{ 0x40, "Seek failure" },
		{ 0x80, "Disk timout (failed to respond)" },
		{ 0xAA, "Drive not ready (hard disk)" },
		{ 0xBB, "Undefined error (hard disk)" },
		{ 0xCC, "Write fault (hard disk)" },
		{ 0xE0, "Statur register error (hard disk)" },
		{ 0xFF, "Sense operation failed (hard disk)" }
	};
	struct errEntry *ep;

	for (ep = errList; ep < arrayLimit(errList); ++ep) {
		if (ep->err == err)
		  return ep->what;
	}
	return "Unknown error";
}

static void rwDiskError(char *rw, off_t sector, int err) {
	printf("\n%s error 0x%02x (%s) at sector %ld absolute\n",
				rw, err, biosDiskError(err), sector);
}

static void readDiskError(off_t sector, int err) {
	rwDiskError("Read", sector, err);
}

/*
static void writeDiskError(off_t sector, int err) {
	rwDiskError("Write", sector, err);
}
*/

static void test() {
	printf("========== Test: printf ===============\n");
	printf("etext: 0x%x, edata: 0x%x, end: 0x%x\n", &etext, &edata, &end);
	printf("%s, %d, 0x%x, %s, %c, 0x%X\n", "abc", 333, 0x9876ABCD, "xxx-yyy", 'a', 0xabcd9876);
	printf("%x, %04X, 0x%x, 0x%x\n", 0xA, 0xF,0xaa55, 0x11112222);

	printf("========== Test: print..Hex ===============\n");
	printByteHex(0xA1);
	print("    ");
	printShortHex(0xA1B2);
	print("    ");
	printlnIntHex(0xA1B2C8D9);
	
	printf("========== Test: Memory Detection ===============\n");
	printLowMem();
	print88Mem();
	printE801Mem();
	printE820Mem();

	println("========== Test end ===============\n");
}

static void determineAvailableMemory() {
	int i, memSize, low, high;

	if ((memSize = detectLowMem()) >= 0) {
		mem[0].base = 0;
		mem[0].size = memSize << 10;

		if (detectE801Mem(&low, &high, true) == 0) {
			mem[1].base = 0x100000;
			mem[1].size = low << 10;

			// if adjacent
			if (low == (15 << 10)) {
				mem[1].size += high << 16;	
			} else {
				mem[2].base = 0x1000000;
				mem[2].size = high << 16;
			}
		} else if ((memSize = detect88Mem()) >= 0) {
			mem[1].base = 0x100000;
			mem[1].size = memSize << 10;
		}
		if (DEBUG) {
			for (i = 0; i < 3; ++i) {
				if (i == 0 || mem[i].base != 0) 
				  printf("Mme[%d] base: 0x%08x, size: %d B\n", i, mem[i].base, mem[i].size);
			}
		}
	}
}

static void copyToFarAway() {
	u32_t memEnd, newAddr, dma64k, oldAddr;

	oldAddr = caddr;
	memEnd = mem[0].base + mem[0].size;
	newAddr = (memEnd - runSize) & ~0x0000FL;
	dma64k = (memEnd - 1) & ~0x0FFFFL;

	/* Check if code and data segment cross a 64K boundary. */
	if (newAddr < dma64k) 
	  newAddr = dma64k - runSize;

	/* Keep pc concurrent. */
	newAddr = newAddr & ~0xFFFFL;

	/* Set the new caddr for relocate. */
	caddr = newAddr;

	/* Copy code and data. */
	rawCopy((char *) newAddr, (char *) oldAddr, runSize); 

	/* Make the copy running. */
	relocate();
}

static void printPartitionEntry(struct partitionEntry **table) {
	struct partitionEntry **pt, *pe;

	printf("  Status  StHead  StSec  StCyl  Type  EdHead  EdSec  EdCyl  LowSec  Count\n");
	for (pt = table; pt < table + NR_PARTITIONS; ++pt) {
		pe = *pt;
		printf("  0x%-4x  %6d  %5d  %5d  %3xh  %6d  %5d  %5d  0x%04x  %5d\n", 
					pe->status, pe->startHead, pe->startSector, pe->startCylinder, 
					pe->type, pe->lastHead, pe->lastSector, pe->lastCylinder,
					pe->lowSector, pe->sectorCount);
	}
}

static struct biosDev {
	char name[8];
	int device, primary, secondary;
} bootDev;

static int getMaster(char *master, struct partitionEntry **table, u32_t pos) {
	int r, n;
	struct partitionEntry *pe, **pt;
	if ((r = readSectors(mon2Abs(master), pos, 1)) != 0)
	  return r;

	pe = (struct partitionEntry *) (master + PART_TABLE_OFF);
	for (pt = table; pt < table + NR_PARTITIONS; ++pt) {
		*pt = pe++;
	}
	if (true) {
		printPartitionEntry(table);
	}

	// Sort partition entries
	n = NR_PARTITIONS;
	do {
		for (pt = table; pt < table + NR_PARTITIONS - 1; ++pt) {
			if (pt[0]->status == INACTIVE_PART || 
						pt[0]->lowSector < pt[1]->lowSector) {
				pe = pt[0];
				pt[0] = pt[1];
				pt[1] = pe;
			}
		}
	} while(--n > 0);

	return 0;
}

static void initialize() {
	int r, p;
	char master[SECTOR_SIZE];
	struct partitionEntry *table[NR_PARTITIONS];
	u32_t masterPos;

	copyToFarAway();

	bootDev.name[0] = 0;
	bootDev.device = device;
	bootDev.primary = -1;
	bootDev.secondary = -1;

	if (device < 0x80) {
		printf("Device is not a hard disk.\n");
		return;
	}

	rawCopy(mon2Abs(&lowSector),
		vec2Abs(&bootPartEntry) + offsetof(struct partitionEntry, lowSector),
		sizeof(lowSector));

	if (true) {
		printf("device: 0x%x\n", device);
		printf("low sector: %x\n", lowSector);
	}

	masterPos = 0;
	
	while (true) {
		// Extract the partition table from the master boot sector.
		if ((r = getMaster(master, table, masterPos)) != 0) {
			readDiskError(masterPos, r);
			exit(1);
		}
	
		// See if we can find "lowSector" back.
		for (p = 0; p < NR_PARTITIONS; ++p) {
			if (lowSector - table[p]->lowSector < table[p]->sectorCount) 
			  break;
		}

		// Found!
		if (lowSector == table[p]->lowSector) {
			if (bootDev.primary < 0)
			  bootDev.primary = p;
			else
			  bootDev.secondary = p;
			break;
		}
		
		if (p == NR_PARTITIONS || 
					bootDev.primary >= 0 ||
					table[p]->status != ACTIVE_PART) {
			bootDev.device = -1;
			return;
		}
	
		// See if the primary partition is sub-partitioned.
		bootDev.primary = p;
		masterPos = table[p]->lowSector;
	}

	strcpy(bootDev.name, "d0p0");
	bootDev.name[1] += (device - 0x80);
	bootDev.name[3] += bootDev.primary;
	if (bootDev.secondary >= 0) {
		strcat(bootDev.name, "s0");
		bootDev.name[5] += bootDev.secondary;
	}
	if (true) {
		printf("bootDev name: %s\n", bootDev.name);
	}
}

void boot() {
	printf("device addr: %x, device: %x\n", &device, device);
	determineAvailableMemory();
	initialize();
	if (DEBUG) {
		test();
		printRangeHex((char *) &x_gdt, 48, 8);
	}
}

