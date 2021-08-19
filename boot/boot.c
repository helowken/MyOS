#include "code.h"
#include "debug.h"
#include "stdarg.h"
#include "stdlib.h"
#include "limits.h"
#include "util.h"
#include "boot.h"
#include "partition.h"

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))

#if DEBUG
static void testPrint() {
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

static void testMalloc() {
	typedef struct {
		char a1;
		short a2;
		int a3;
		long a4;
		char *a5;
	} AA;

	AA *pp[10], *p;
	for (int i = 0; i < 10; ++i) {
		p = malloc(sizeof(AA));
		p->a1 = 'a' + i;
		p->a2 = 1 + i;
		p->a3 = 10 + i;
		p->a4 = 100 + i;
		p->a5 = "abcdefg";
		pp[i] = p;
	}
	for (int i = 0; i < 10; ++i) {
		p = pp[i];
		printf("a1: %c, a2: %d, a3: %d, a4: %d, a5: %s\n",
			p->a1, p->a2, p->a3, p->a4, p-> a5);
	}
	for (int i = 0; i < 10; ++i) {
		p = realloc(pp[i], sizeof(AA));
		pp[i] = p;
		printf("a1: %c, a2: %d, a3: %d, a4: %d, a5: %s\n",
			p->a1, p->a2, p->a3, p->a4, p-> a5);
	}
	for (int i = 0; i < 10; ++i) {
		free(pp[i]);
	}
}
#endif

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

static void determineAvailableMemory() {
	int i, memSize, low, high;

	if ((memSize = detectLowMem()) >= 0) {
		memList[0].base = 0;
		memList[0].size = memSize << 10;

		if (detectE801Mem(&low, &high, true) == 0) {
			memList[1].base = 0x100000;
			memList[1].size = low << 10;

			// if adjacent
			if (low == (15 << 10)) {
				memList[1].size += high << 16;	
			} else {
				memList[2].base = 0x1000000;
				memList[2].size = high << 16;
			}
		} else if ((memSize = detect88Mem()) >= 0) {
			memList[1].base = 0x100000;
			memList[1].size = memSize << 10;
		}
		if (DEBUG) {
			for (i = 0; i < 3; ++i) {
				if (i == 0 || memList[i].base != 0) 
				  printf("Mme[%d] base: 0x%08x, size: %d B\n", i, memList[i].base, memList[i].size);
			}
		}
	}
}

static void copyToFarAway() {
	u32_t memEnd, newAddr, dma64k, oldAddr;

	oldAddr = caddr;
	memEnd = memList[0].base + memList[0].size;
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
	debug(printPartitionEntry(table));

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

	debug(printf("device: 0x%x\n", device));
	debug(printf("low sector: %x\n", lowSector));

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

	debug(printf("bootDev name: %s\n", bootDev.name));
	debug(testPrint());
	debug(printRangeHex((char *) &x_gdt, 48, 8));
	debug(testMalloc());
}

enum ReservedNameEnum {
	R_NULL, R_ROOT, R_CTTY, R_DELAY, R_ECHO, R_EXIT, R_HELP,
	R_LS, R_MENU, R_OFF, R_SAVE, R_SET, R_TRAP, R_UNSET
};

char ReservedNames[][6] = {
	"", "boot", "ctty", "delay", "echo", "exit", "help",
	"ls", "menu", "off", "save", "set", "trap", "unset"
};

int isReserved(char *s) {
	int r;
	for (r = R_ROOT; r <= R_UNSET; ++r) {
		if (strcmp(ReservedNames[r], s) == 0)
		  return true;
	}
	return false;
}

static char *copyStr(char *s) {
	char *c;

	if (*s == '\0')
	  return NULL;
	c = malloc((strlen(s) + 1) * sizeof(char));
	strcpy(c, s);
	return c;
}

static void sfree(char *s) {
	if (s != NULL)
	  free(s);
}

static Environment **searchEnv(char *name) {
	Environment **aenv = &env;
	while (*aenv != NULL && strcmp((*aenv)->name, name) != 0) {
		aenv = &(*aenv)->next;
	}
	return aenv;
}

static bool isDefault(Environment *e) {
	return (e->flags & E_SPECIAL) && e->defValue == NULL;
}

/*
 *	Change the value of an environment variable.
 *	Returns the flags of the variable if you are not allowed to change it, 0 otherwise.
 */
static int setEnv(int flags, char *name, char *arg, char *value) {
	Environment **aenv, *e;
	if (*(aenv = searchEnv(name)) == NULL) {
		if (isReserved(name))
		  return E_RESERVED;
		e = malloc(sizeof(*e));
		e->name = copyStr(name);
		e->flags = flags;
		e->defValue = NULL;
		e->next = NULL;
		*aenv = e;
	} else {
		e = *aenv;
		if ((e->flags & E_SPECIAL) &&
					(e->flags & E_FUNCTION) != (flags & E_FUNCTION))
		  return e->flags;

		e->flags = (e->flags | E_STICKY) & flags;
		if (isDefault(e)) 
		  e->defValue = e->value;
		else
		  sfree(e->value);

		sfree(e->arg);
	}
	e->arg = copyStr(arg);
	e->value = copyStr(value);

	return 0;
}

static int setVar(int flags, char *name, char *value) {
	return setEnv(flags, name, NULL, value);
}

/*
 * Remove a variable from the environment.
 * A special variable is reset to its default value.
static void unset(char *name) {
	environment **aenv, *e;

	if ((e = *(aenv = searchEnv(name))) == NULL)
	  return;

	if (e->flags & E_SPECIAL) {
		if (e->defValue != NULL) {
			sfree(e->arg);
			e->arg = NULL;
			sfree(e->value);
			e->value = e->defValue;
			e->defValue = NULL;
		}
	} else {
		sfree(e->name);
		sfree(e->arg);
		sfree(e->value);
		*aenv = e->next;
		free(e);
	}
}
 */

/*
 * Transform a long number to ascii at base b, (b >= 8).
 */
static char *ul2a(u32_t n, unsigned b) {
	static char num[(CHAR_BIT * sizeof(n) + 2) / 3 + 1];
	char *a = arrayLimit(num) - 1;
	static char hex[16] = "0123456789ABCDEF";
	do {
		*--a = hex[n % b];	
	} while ((n/=b) > 0);
	return a;
}

/*
 * Transform a long number to ascii at base 10.
static char *ul2a10(u32_t n) {
	return ul2a(n, 10);
}
 */

static void getParameters() {
	char params[SECTOR_SIZE + 1];
	int r, videoMode;
	Memory *mp, *mpEnd;
	static char busType[][4] = {
		"xt", "at", "mc"
	};
	static char videoType[][4] = {
		"mda", "cga", "ega", "ega", "vga", "vga"
	};
	static char videoChrome[][6] = {
		"mono", "color"
	};
	videoMode = getVideoMode();

	debug(printf("Bus type: %x\n", getBus()));
	debug(printf("Video mode: %d\n", getVideoMode()));

	setVar(E_SPECIAL|E_VAR|E_DEV, "rootdev", "ram");
	setVar(E_SPECIAL|E_VAR|E_DEV, "ramimagedev", "bootdev");
	setVar(E_SPECIAL|E_VAR, "ramsize", "0");
	//setVar(E_SPECIAL|E_VAR, "processor", );
	setVar(E_SPECIAL|E_VAR, "bus", busType[getBus()]);
	setVar(E_SPECIAL|E_VAR, "video", videoType[videoMode]);
	setVar(E_SPECIAL|E_VAR, "chrome", videoChrome[videoMode]);
	params[0] = 0;
	for (mp = memList, mpEnd = arrayLimit(memList); mp < mpEnd; ++mp) {
		if (mp->size > 0) {
			if (params[0] != 0)
			  strcat(params, ",");
			strcat(params, ul2a(mp->base, 0x10));
			strcat(params, ":");
			strcat(params, ul2a(mp->size, 0x10));
		}
	}
	setVar(E_SPECIAL|E_VAR, "memory", params);

	setVar(E_SPECIAL|E_VAR, "label", "AT");
	setVar(E_SPECIAL|E_VAR, "controller", "c0");
	
	// Variables boot needs:
	setVar(E_SPECIAL|E_VAR, "image", "boot/image");
	setVar(E_SPECIAL|E_FUNCTION, "leader", 
		"echo --- Welcome to MINIX3. This is the boot monitor. ---\\n");
	setVar(E_SPECIAL|E_FUNCTION, "main", "menu");
	setVar(E_SPECIAL|E_FUNCTION, "trailer", "");

	setEnv(E_RESERVED|E_FUNCTION, NULL, "=,Start MINIX", "boot");
	
	// Tokneize boot params sector.
	if ((r = readSectors(mon2Abs(params), lowSector + PARAM_SECTOR, 1)) != 0) {
		readDiskError(lowSector + PARAM_SECTOR, r);
		exit(1);
	}
	params[SECTOR_SIZE] = 0;

}

void boot() {
	debug(printf("device addr: %x, device: %x\n", &device, device));

	determineAvailableMemory();
	initialize();
	getParameters();


}

