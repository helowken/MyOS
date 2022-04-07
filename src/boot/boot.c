#include "code.h"
#include "debug.h"
#include "stdarg.h"
#include "stdlib.h"
#include "limits.h"
#include "util.h"
#include "errno.h"
#include "minix/dmap.h"
#include "ibm/partition.h"
#include "sys/stat.h"
#include "rawfs.h"

#undef EXTERN
#define EXTERN			/* Empty, allocate space for variables */
#include "boot.h"

static char *version = "2.20";
static int fsOK = -1;	/* File system state. Initially unknown. */
static int activate;

static struct biosDev {
	char name[8];
	int device, primary, secondary;
} bootDev, tmpDev;

typedef struct Token {
	struct Token *next;
	char	*token;
} Token;

static Token *cmds;
static bool tokErr = false;

static char *timerHandler;
static u32_t timeBase, timeout;

static char *biosDiskError(int err) {
/* BIOS INT 13h errors */
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

void readDiskError(off_t sector, int err) {
	rwDiskError("Read", sector, err);
}

static void writeDiskError(off_t sector, int err) {
	rwDiskError("Write", sector, err);
}

/* Device numbers of standard MINIX devices. */
#define DEV_FD0		0x0200
static dev_t devCNd0[] = {	/* Refer to dmap.c */
	0x0300,		/* /dev/c0 */
	0x0800,		/* /dev/c1 */
	0x0A00,		/* /dev/c2 */
	0x0C00,		/* /dev/c3 */
	0x1000		/* /dev/random */
};
#define MINOR_d0p0s0	128

dev_t name2Dev(char *name) {
/* Translate , say, /dev/c0d0p2 to a device number. If the name can't be
 * found on the boot device, then do some guesswork. The global structure
 * "tmpDev" will be filled in based on the name, so that "boot d1p0" knows
 * what device to boot without interpreting device numbers.
 */
	dev_t dev;
	ino_t ino;
	char *n;
	struct stat st;
	int controllerNum, blockSize;

	/* "boot *d0p2" means: make partition 2 active before you boot it. */
	if ((activate = (name[0] == '*')))
	  ++name;
	
	/* The special name "bootdev" must be translated to the boot device. */
	if (strcmp(name, "bootdev") == 0) {
		if (bootDev.device == -1) {
			printf("The boot device could not be named.\n");
			errno = 0;
			return -1;
		}
		name = bootDev.name;
	}

	/* If our boot device doesn't have a file system, or we want to know
	 * what a name means for the BIOS, then we need to interpret the
	 * device name ourselves: "fd" = floppy, "c0d0" = hard disk, etc.
	 */
	tmpDev.device = tmpDev.primary = tmpDev.secondary = -1;
	dev = -1;
	n = name;
	if (strncmp(n, "/dev/", 5) == 0)
	  n += 5;
	if (strcmp(n, "ram") == 0) {
		dev = DEV_RAM;
	} else if (strcmp(n, "boot") == 0) {
		dev = DEV_BOOT;
	} else if (n[0] == 'f' && n[1] == 'd' && numeric(n + 2)) {
		/* Floppy. */
		tmpDev.device = a2l(n + 2);
		dev = DEV_FD0 + tmpDev.device;
	} else {
		/* Hard disk. */
		if (n[0] == 'c' && between('0', n[1], '4')) {	/* c[0-3] */
			controllerNum = n[1] - '0';		
			tmpDev.device = 0;
			n += 2;
		}
		if (n[0] == 'd' && between('0', n[1], '7')) {	/* d[0-7] */
			tmpDev.device = n[1] - '0';
			n += 2;
			if (n[0] == 'p' && between('0', n[1], '3')) {	/* p[0-3] */
				tmpDev.primary = n[1] - '0';
				n += 2;
				if (n[0] == 's' && between('0', n[1], '3')) {	/* s[0-3] */
					tmpDev.secondary = n[1] - '0';
					n+= 2;
				}
			}
		}
		if (*n == 0) {
			dev = devCNd0[controllerNum];
			/* Refer to wPrepare() in at_wini.c */
			if (tmpDev.secondary < 0) {
				dev += tmpDev.device * (NR_PARTITIONS + 1) + 
					(tmpDev.primary + 1);
			} else {
				dev += MINOR_d0p0s0 + 
					(tmpDev.device * NR_PARTITIONS + tmpDev.primary) * NR_PARTITIONS +
					tmpDev.secondary;

			}
			tmpDev.device += 0x80;
		}
	}

	/* Look the name up on the boot device for the UNIX device number. */
	if (fsOK == -1) 
	  fsOK = rawSuper(&blockSize) != 0;
	if (fsOK) {
		/* The current working directory is "/dev". */
		ino = rawLookup(rawLookup(ROOT_INO, "dev"), name);
		if (ino != 0) {
			/* Name has been found, extract the device number. */
			rawStat(ino, &st);
			if (!S_ISBLK(st.st_mode)) {
				printf("%s is not a block device\n", name);
				errno = 0;
				return (dev_t) -1;
			}
			dev = st.st_rdev;
		}
	}

	if (tmpDev.primary < 0)
	  activate = 0;

	if (dev == -1) {
		printf("Can't recognize '%s' as a device\n", name);
		errno = 0;
	}
	return dev;
}

void readBlock(Off_t blockNum, char *buf, int blockSize) {
/* Read blocks for the rawfs package. */
	int r, sectors;
	u32_t pos;

	if (!blockSize) {
		printf("blockSize: 0\n");
		exit(1);
	}

	sectors = RATIO(blockSize);
	pos = lowSector + blockNum * sectors;
	if ((r = readSectors(mon2Abs(buf), pos, sectors)) != 0) {
		readDiskError(pos, r);
		exit(1);
	}
}

static void determineAvailableMemory() {
	int i, memSize, low, high;

	if ((memSize = detectLowMem()) >= 0) {
		memList[0].base = 0;
		memList[0].size = memSize << 10;

		if (detectE801Mem(&low, &high, true) == 0) {
			memList[1].base = 0x100000;
			memList[1].size = low << 10;

			if (low == (15 << 10)) {
				/* memList[1] and memList[2] is adjacent, there is no hole between 15M and 16M. 
				 * So we merge these two mem chunks together. */
				memList[1].size += high << 16;	
			} else {
				/* There is a hole between 15M and 16M */
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

	/* Copy the boot program to the far end of low memory (below 640K), 
	 * this must be done to get out of the way of Minix (starts from 2K), 
	 * and to put the data area cleanly inside a 64K chunk if using 
	 * BIOS I/O (no DMA problems).
	 */
	oldAddr = caddr;
	memEnd = memList[0].base + memList[0].size;
	newAddr = (memEnd - runSize) & ~0x0000FL;
	dma64k = (memEnd - 1) & ~0x0FFFFL;

	/* Check if code and data segment cross a 64K boundary. */
	if (newAddr < dma64k)	/* We need to copy with stack data */
	  newAddr = dma64k - runSize;

	/* Keep program counter concurrent. */
	newAddr = newAddr & ~0xFFFFL;

	/* Set the new caddr for relocate. */
	caddr = newAddr;

	/* Copy code and data. */
	rawCopy((char *) newAddr, (char *) oldAddr, runSize); 

	/* Make the copy running. */
	relocate();

	/* If we have memory to spare, keep monitor in memList[0]. */
	if (memList[1].size > 512*1024L)
	  memList[0].size = newAddr;

	/*
	 * BIOS IVT (Interrupt Vector Table) + BDA (BIOS data area) is about 1.5KB (see "BUFFER" in masterboot.asm).
	 * We simply use 2KB to align it.
	 */
	memList[0].base += CLICK_SIZE << 1;
	memList[0].size -= CLICK_SIZE << 1;
}

static int getMaster(char *master, PartitionEntry **table, u32_t pos) {
/* Read a master boot sector and its partition table. */
	int r, n;
	PartitionEntry *pe, **pt;
	if ((r = readSectors(mon2Abs(master), pos, 1)) != 0)
	  return r;

	pe = (PartitionEntry *) (master + PART_TABLE_OFF);
	for (pt = table; pt < table + NR_PARTITIONS; ++pt) {
		*pt = pe++;
	}

	/* Sort partition entries */
	n = NR_PARTITIONS;
	do {
		for (pt = table; pt < table + NR_PARTITIONS - 1; ++pt) {
			if (pt[0]->type == NO_PART || 
					pt[0]->lowSector > pt[1]->lowSector) {
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
	PartitionEntry *table[NR_PARTITIONS];
	u32_t masterPos;

	copyToFarAway();

	bootDev.name[0] = 0;
	bootDev.device = device;
	bootDev.primary = -1;
	bootDev.secondary = -1;

	if (device < 0x80) {
		/* Floppy. */
		strcpy(bootDev.name, "fd0");
		bootDev.name[2] += bootDev.device;
		return;
	}

	/* Disk: Get the partition table from the very first sector, and
	 * determine the partition we booted from using the information from
	 * the booted partition entry as passed on by the bootstrap.
	 * All we need from it is the partition offset.
	 */
	rawCopy(mon2Abs(&lowSector),
		vec2Abs(&bootPartEntry) + offsetof(PartitionEntry, lowSector),
		sizeof(lowSector));

	masterPos = 0;
	
	while (true) {
		/* Extract the partition table from the master boot sector. */
		if ((r = getMaster(master, table, masterPos)) != 0) {
			readDiskError(masterPos, r);
			exit(1);
		}
	
		/* See if we can find "lowSector" back. */
		for (p = 0; p < NR_PARTITIONS; ++p) {
			if (lowSector - table[p]->lowSector < table[p]->sectorCount) 
			  break;
		}

		/* Found! */
		if (lowSector == table[p]->lowSector) {
			if (bootDev.primary < 0)
			  bootDev.primary = p;
			else
			  bootDev.secondary = p;
			break;
		}
		
		if (p == NR_PARTITIONS || 
					bootDev.primary >= 0 ||
					table[p]->type != MINIX_PART) {
			/* The boot partition cannot be named, this only means
			 * that "bootdev" doesn't work.
			 */
			bootDev.device = -1;
			return;
		}
	
		/* See if the primary partition is sub-partitioned. */
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
}

enum ReservedNameEnum {
	R_NULL, R_BOOT, R_CTTY, R_DELAY, R_ECHO, R_EXIT, R_HELP,
	R_LS, R_MENU, R_OFF, R_SAVE, R_SET, R_TRAP, R_TEST, R_UNSET 
};

char ReservedNames[][6] = {
	"", "boot", "ctty", "delay", "echo", "exit", "help",
	"ls", "menu", "off", "save", "set", "trap", "test", "unset", 
};

int reserved(char *s) {
	int r;
	for (r = R_BOOT; r <= R_UNSET; ++r) {
		if (strcmp(ReservedNames[r], s) == 0)
		  return r;
	}
	return R_NULL;
}

static char *copyStr(char *s) {
	char *c;

	if (s == NULL || *s == 0)
	  return NULL;
	c = malloc((strlen(s) + 1) * sizeof(char));
	strcpy(c, s);
	return c;
}

static void sfree(char *s) {
	if (s != NULL)
	  free(s);
}

#define getEnv(name)	(*searchEnv(name))

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

static char *addPtr;

static void addParam(char *n) {
	while (n != NULL && *n != 0 && *addPtr != 0) {
		*addPtr++ = *n++;
	}
}

static void saveParameters() {
	Environment *e;
	char params[SECTOR_SIZE + 1];
	int r;

	memset(params, ';', SECTOR_SIZE);
	params[SECTOR_SIZE] = 0;
	addPtr = params;

	for (e = env; e != NULL; e = e->next) {
		if (e->flags & E_RESERVED || isDefault(e))
		  continue;
		
		addParam(e->name);
		if (e->flags & E_FUNCTION) {
			addParam("(");
			addParam(e->arg);
			addParam(")");
		} else {
			/*
			 * if e == DEV | SPECIAL: then 'd' is no need to save
			 * if e != DEV: then 'd' is no need to save
			 * if e == DEV: then 'd' is needed to save
			 */
			addParam((e->flags & (E_DEV | E_SPECIAL)) != E_DEV ? "=" : "=d ");
		}
		addParam(e->value);
		if (*addPtr == 0) {
			printf("The environment is too big\n");
			return;
		}
		*addPtr++ = ';';
	}

	/* Save the parameters on disk. */
	if ((r = writeSectors(mon2Abs(params), lowSector + PARAM_SECTOR, 1)) != 0) {
		writeDiskError(lowSector + PARAM_SECTOR, r);
		printf("Can't save environment\n");
	}
}

static void showEnv() {
	Environment *e;
	unsigned more = 0;
	int c;

	for (e = env; e != NULL; e = e->next) {
		if (e->flags & E_RESERVED)
		  continue;

		if (e->flags & E_FUNCTION) {
			printf("%s(%s) %s\n", e->name, e->arg, e->value);
		} else {
			printf(isDefault(e) ? "%s = (%s)\n" : "%s = %s\n", 
						e->name, e->value);
		}
		
		if (e->next != NULL && ++more % 20 == 0) {
			printf("More? ");
			c = getch();
			if (c == ESC || c > ' ') {
				putch('\n');
				if (c > ' ')
				  ungetch(c);
				break;
			}
			/*
			 * Delete previous displayed "More? "
			 * 1. "More? " length: 6
			 * 2. "\b\b\b\b\b\b":	go back to the start position
			 * 3. "      ":			output 6 whitespaces to replace "More? "
			 * 4. "\b\b\b\b\b\b":	go back to the start position again
			 */
			printf("\b\b\b\b\b\b      \b\b\b\b\b\b");		
		}
	}
}

char *getVarValue(char *name) {
	Environment *e = getEnv(name);		
	return e == NULL || !(e->flags & E_VAR) ? NULL : e->value;
}

static char *getFuncBody(char *name) {
	Environment *e = getEnv(name);
	return e == NULL || !(e->flags & E_FUNCTION) ? NULL : e->value;
}

static int setEnv(int flags, char *name, char *arg, char *value) {
/* Change the value of an environment variable.
 * Returns the flags of the variable if you are not allowed to change it, 0 otherwise.
 */
	Environment **aenv, *e;
	if (*(aenv = searchEnv(name)) == NULL) {
		if (reserved(name))
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

		e->flags = (e->flags & E_STICKY) | flags;
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

static void unset(char *name) {
/* Remove a variable from the environment.
 * A special variable is reset to its default value.
 */
	Environment **aenv, *e;

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

static bool sugar(char *tok) {
	return strchr("=(){};\n", tok[0]) != NULL;
}

static void printTokens() {
	Token *p = cmds;
	int i = 0;
	while (p != NULL) {
		if (i++ > 0)
		  printf(",\t");
		printf("%s", p->token);
		p = p->next;
	}
	printf("\n");
}

static char *oneToken(char **aline) {
	char *line = *aline;
	size_t n;
	char *tok;

	/* Skip spaces and runs of newlines. */
	while (*line == ' ' || (*line == '\n' && line[1] == '\n')) {
		++line;
	}
	*aline = line;

	if ((unsigned) *line < ' ' && *line != '\n')
	  return NULL;

	if (*line == '(') {
		/* Function argument, anything goes but () must match. */
		int depth = 0;
		while ((unsigned) *line >= ' ') {
			if (*line == '(') 
			  ++depth;
			if (*line++ == ')' && --depth == 0)
			  break;
		}
	} else if (sugar(line)) {
		/* Single character token. */
		++line;	
	} else {
		/* Multi character token. */
		do {
			++line;
		} while ((unsigned) *line > ' ' && !sugar(line));
	}
	n = line - *aline;
	tok = malloc((n + 1) * sizeof(char));
	memcpy(tok, *aline, n);
	tok[n] = 0;
	if (tok[0] == '\n')		/* ';' same as '\n' */
	  tok[0] = ';';
	*aline = line;
	return tok;
}

static Token **tokenize(Token **acmds, char *line) {
	char *tok;
	Token *newCmd;

	while ((tok = oneToken(&line)) != NULL) {
		newCmd = malloc(sizeof(*newCmd));
		newCmd->token = tok;
		newCmd->next = *acmds;
		*acmds = newCmd;
		acmds = &newCmd->next;
	}
	return acmds;
}

static u32_t getProcessor() {
	/* TODO */
	return 586;
}

static void getParameters() {
	char params[SECTOR_SIZE + 1];
	int r, videoMode;
	Memory *mp, *mpEnd;
	Token **acmds;
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

	setVar(E_SPECIAL|E_VAR|E_DEV, "rootdev", "ram");
	setVar(E_SPECIAL|E_VAR|E_DEV, "ramimagedev", "bootdev");
	setVar(E_SPECIAL|E_VAR, "ramsize", "0");
	setVar(E_SPECIAL|E_VAR, "processor", ul2a10(getProcessor()));
	setVar(E_SPECIAL|E_VAR, "bus", busType[getBus()]);
	setVar(E_SPECIAL|E_VAR, "video", videoType[videoMode]);
	setVar(E_SPECIAL|E_VAR, "chrome", videoChrome[videoMode & 1]);
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
	
	/* Variables boot needs: */
	setVar(E_SPECIAL|E_VAR, "image", "boot/image");
	setVar(E_SPECIAL|E_FUNCTION, "leader", 
		"echo --- Welcome to MINIX3. This is the boot monitor. ---\\n");
	setVar(E_SPECIAL|E_FUNCTION, "main", "menu");
	/* Set trailer function body as empty. */
	setVar(E_SPECIAL|E_FUNCTION, "trailer", ";"); 

	setEnv(E_RESERVED|E_FUNCTION, NULL, "=,Start MINIX", "boot");

	/* Tokneize boot params sector. */
	if ((r = readSectors(mon2Abs(params), lowSector + PARAM_SECTOR, 1)) != 0) {
		readDiskError(lowSector + PARAM_SECTOR, r);
		exit(1);
	}
	params[SECTOR_SIZE] = 0;
	acmds = tokenize(&cmds, params);

	/* Stuff the default action into the command chain. */
	tokenize(acmds, ":;leader;main");
}

static void help() {
	struct help {
		char *thing;
		char *help;
	} *pi;
	static struct help info[] = {
		{ NULL,	"Names:" },
		{ "rootdev",					"Root device" },
		{ "ramimagedev",				"Device to use as RAM disk image " },
		{ "ramsize",					"RAM disk size (if no image device) " },
		{ "bootdev",					"Special name for the boot device" },
		{ "fd0, d0p2, c0d0p1s0",		"Devices (as in /dev)" },
		{ "image",						"Name of the boot image to use" },
		{ "main",						"Startup function" },
		{ "bootdelay",					"Delay in msec after loading image" },
		{ NULL,	"Commands:" },
		{ "name = [device] value",		"Set environment variable" },
		{ "name() { ... }",				"Define function" },
		{ "name(key, text) { ... }",	"A menu option like: minix(=,Start MINIX) {boot}" },
		{ "name",						"Call function" },
		{ "boot [device]",				"Boot Minix or another O.S." },
		{ "ctty [line]",				"Duplicate to serial line" },
		{ "delay [msec]",				"Delay (500 msec default)" },
		{ "echo word",					"Display the words" },
		{ "ls [directory]",				"List contents of directory" },
		{ "menu",						"Show menu and choose menu option" },
		{ "save /set",					"Save or show environment" },
		{ "trap msec command",			"Schedule command " },
		{ "unset name ...",				"Unset variable or set to default" },
		{ "exit / off",					"Exit the Monitor / Power off" },
		{ "test",						"Test functions in this program" }
	};

	for (pi = info; pi < arrayLimit(info); ++pi) {
		if (pi->thing != NULL)
		  printf("    %-24s- ", pi->thing);
		printf("%s\n", pi->help);
	}
}

static char *popToken() {
	Token *cmd = cmds;
	char *tok = cmd->token;
	cmds = cmd->next;
	free(cmd);
	return tok;
}

static void voidToken() {
	free(popToken());
}

static u32_t milliTime() {
	return getTick() * MSEC_PER_TICK;
}

static u32_t milliTimeSince(u32_t base) {
	u32_t msecOfDay = TICKS_PER_DAY * MSEC_PER_TICK;
	return (milliTime() + msecOfDay - base) % msecOfDay;
}

static void unschedule() {
	if (timerHandler != NULL) {
		free(timerHandler);
		timerHandler = NULL;
	}
}

static void schedule(long msec, char *cmd) {
	unschedule();
	timerHandler = cmd;
	timeBase = milliTime();
	timeout = msec;
}

bool expired() {
	return timerHandler != NULL && 
		milliTimeSince(timeBase) >= timeout;
}

static bool interrupt() {
	if (escape()) {
		printf("[ESC]\n");
		tokErr = true;
		return true;
	}
	return false;
}

void delay(u32_t msec) {
	u32_t base;

	if (msec == 0)
	  return;
	base = milliTime();
	do {
		pause();
	} while (!interrupt() && !expired() && milliTimeSince(base) < msec);
}


typedef enum MenuFuncType {
	NOT_FUNC, SELECT, DEF_FUNC, USER_FUNC 
} MenuFuncType;

static MenuFuncType getMenuFuncType(Environment *e) {
/* a()ls	: NOT_FUNC
 * a(aa)ls	: SELECT
 * a(a,b)ls	: USER_FUNC / DEF_FUNC
 */
	if (!(e->flags & E_FUNCTION) || 
				e->arg == NULL || 
				e->arg[0] == 0)
	  return NOT_FUNC;
	if (e->arg[1] != ',')
	  return SELECT;
	return e->flags & E_RESERVED ? DEF_FUNC : USER_FUNC;
}

static void menu() {
	int c;
	bool defaultMenu = true;
	char *choose = NULL;
	Environment *e;

	for (e = env; e != NULL; e = e->next) {
		if (getMenuFuncType(e) == USER_FUNC) {
			defaultMenu = false;
			break;
		}
	}

	printf("\nHit a key as follows:\n\n");

	for (e = env; e != NULL; e = e->next) {
		switch (getMenuFuncType(e)) {
			case NOT_FUNC:
				break;
			case DEF_FUNC:
				if (!defaultMenu)
				  break;
			case USER_FUNC:
				printf("	%c  %s\n", e->arg[0], &e->arg[2]);
				break;
			case SELECT:
				printf("	%c  Select %s kernel\n", e->arg[0], e->name);
				break;
		}
	}

	/* Wait for a keypress. */
	do {
		c = getch();
		if (interrupt() || expired())
		  return;
		unschedule();
		for (e = env; e != NULL; e = e->next) {
			switch (getMenuFuncType(e)) {
				case NOT_FUNC:
					break;
				case DEF_FUNC:
					if (!defaultMenu)
					  break;
				case USER_FUNC:
				case SELECT:
					if (c == e->arg[0])
					  choose = e->value;
					break;
			}
			if (choose != NULL)
			  break;
		}
	} while (choose == NULL);

	/* Execute the chosen function. */
	printf("%c\n", c);
	tokenize(&cmds, choose);
}

void parseCode(char *code) {
	if (cmds != NULL && cmds->token[0] != ';')
	  tokenize(&cmds, ";");
	tokenize(&cmds, code);
}

static void execute() {
	Token *second, *third, *fourth, *sep;
	char *name;
	int res;
	size_t n = 0;

	if (tokErr) {
		while (cmds != NULL)
		  voidToken();
		return;
	}

	if (expired()) {
		parseCode(timerHandler);
		unschedule();
	}

	for (sep = cmds; sep != NULL && sep->token[0] != ';'; sep = sep->next) {
		++n;
	}

	name = cmds->token;
	res = reserved(name);
	if ((second = cmds->next) != NULL &&
			(third = second->next) != NULL)
	  fourth = third->next;

	if (n == 0) {
		/* Null command. */
		voidToken();
		return;
	} else if ((n == 3 || n == 4) && 
				!sugar(name) &&
				second->token[0] == '=' &&
				!sugar(third->token) &&
				(n == 3 || (n == 4 && 
							third->token[0] == 'd' &&
							!sugar(fourth->token)))
	) {
		/* name = [device] value. */
		char *value = third->token;
		int flags = E_VAR;
	
		if (n == 4) {
			value = fourth->token;
			flags |= E_DEV;
		}
		if ((flags = setVar(flags, name, value)) != 0) {
			printf("%s is a %s\n", name,
				flags & E_RESERVED ? "reserved word" : "special function");
			tokErr = true;
		}
		while (cmds != sep) {
			voidToken();
		}
		return;
	} else if (n >= 3 && 
				!sugar(name) &&
				second->token[0] == '(') {
		 /* Function definition. */
		 /* For example: */
		 /* 1. a() ls */
		 /* 2. a() {ls /dev} */
		Token *func;
		char c;
		int flags, depth;
		char *body;
		size_t len;

		sep = func = third;
		depth = 0;
		len = 1;
		while (sep != NULL) {
			if ((c = sep->token[0]) == ';' && depth == 0)
			  break;
			len += strlen(sep->token) + 1;
			sep = sep->next;
			if (c == '{')
			  ++depth;
			else if (c == '}' && --depth == 0)
			  break;
		}
		body = malloc(len * sizeof(char));
		*body = 0;
		
		while (func != sep) {
			strcat(body, func->token);
			if (!sugar(func->token) && 
						!sugar(func->next->token))
			  strcat(body, " ");
			func = func->next;
		}
		/* remove ')' from the second token "(...)" */
		second->token[strlen(second->token) - 1] = 0;	

		if (depth != 0) {
			printf("Missing '%c'\n", depth < 0 ? '{' : '}');
			tokErr = true;
		} else if (
				/* remove '(' from the second token "(..." */
				(flags = setEnv(E_FUNCTION, name, second->token + 1, body)) != 0
		) {	
			printf("%s is a %s\n", name, 
						flags & E_RESERVED ? "reserved word" : "special variable");
			tokErr = true;
		}
		while (cmds != sep) {
			voidToken();
		}
		free(body);
		return;
	} else if (name[0] == '{') {
		/* Grouping */
		Token *p = cmds->next;
		char *t;
		int depth = 1;
		
		/* Find and remove matching '}' */
		while (p != NULL) {
			t = p->token;
			if (t[0] == '{')
			  ++depth;
			else if (t[0] == '}' && --depth == 0) {
				t[0] = ';';
				break;
			}
			p = p->next;
		}
		voidToken();
		return;
	} else if (interrupt()) {
		return;
	} else if (n >= 1 && 
				(res == R_UNSET || res == R_ECHO)) {
		/* unset name ..., echo word ... */
		char *arg, *p;
		Environment *e;
		arg = popToken(); /* arg = "unset" or "echo" */
		
		while (true) {
			free(arg);
			if (cmds == sep)
			  break;
			arg = popToken();
			if (res == R_UNSET) {
				/* unset arg */
				unset(arg);
			} else {
				/* echo arg */
				p = arg;
				if (*p == '$') {
					if (*++p == 0)  {
						putch('$');
					} else {
						e = getEnv(p);
						if (e != NULL)  {
							if (e->flags & E_FUNCTION)
							  printf("%s(%s) %s", e->name, e->arg, e->value);
							else
							  printf("%s", e->value);
						}
					}
				} else {
					while (*p != 0) {
						if (*p != '\\') {
						  putch(*p);
						} else {
							switch (*++p) {
								case 0:
									if (cmds == sep)
									  return;
									continue;
								case 'n':
									putch('\n');
									break;
								case 'v':
									printf(version);
									break;
								case 'c':
									clearScreen();
									break;
								case 'w':
									while (true) {
										if (interrupt())
										  return;
										if (getch() == '\n')
										  break;
									}
									break;
								default:
									putch(*p);
							}
						}
						++p;
					}
				}
				putch(cmds != sep ? ' ' : '\n');
			}
		}
		return;
	} else if (n == 2 && 
				res == R_BOOT && 
				second->token[0] == '-') {
		/* boot -opts */
		static char *optsVar = "bootopts";
		setVar(E_VAR, optsVar, second->token);
		voidToken();
		voidToken();
		bootMinix();
		unset(optsVar);
		return;
	} else if (n == 2 && 
				(res == R_BOOT ||	/* boot device */
				 res == R_CTTY ||	/* ctty */
				 res == R_DELAY ||	/* delay msec */
				 res == R_LS)) {	/* ls dir */
		/*TODO
		if (res == R_BOOT)
		  bootDevice(second->token);
		else if (res == R_CTTY)
		  ctty(second->token);
		else if (res == R_LS)
		  ls(second->token);
		else 
		*/
		if (res == R_DELAY)
		  delay(a2l(second->token));
		voidToken();
		voidToken();
		return;
	} else if (n == 3 && 
				res == R_TRAP &&
				numeric(second->token)) {
		/* trap msec command */
		long msec = a2l(second->token);
		voidToken();
		voidToken();
		schedule(msec, popToken());
		return;
	} else if (n == 1) {
		/* Simple command. */
		char *body;
		bool ok = false;
		
		name = popToken();
		switch (res) {
			case R_BOOT:
				bootMinix();
				ok = true;
				break;
			case R_DELAY:
				delay(500);
				ok = true;
				break;
			case R_LS:
				/* TODO ls(NULL); */
				ok = true;
				break;
			case R_MENU:
				menu();
				ok = true;
				break;
			case R_SAVE:
				saveParameters();
				ok = true;
				break;
			case R_SET:
				showEnv();
				ok = true;
				break;
			case R_HELP:
				help();
				ok = true;
				break;
			case R_EXIT:
				exit(0);
				ok = true;
				break;
			case R_OFF:
				/* TODO off(); */
				ok = true;
				break;
			case R_TEST:
				// For testing */
				name2Dev("c0d0p0s0");
				ok = true;
				break;
		}
		if (strcmp(name, ":") == 0)
		  ok = true;

		if (!ok && (body = getFuncBody(name)) != NULL) {
			tokenize(&cmds, body);
			ok = true;
		}
		if (!ok)
		  printf("%s: unknown function", name);
		free(name);
		if (ok)
		  return;
	} else {
		/* Syntax error. */
		printf("Can't parse:");
		while (cmds != sep) {
			printf(" %s", cmds->token);
			voidToken();
		}
	}
	/* Getting here means that the command is not understand. */
	printf("\nTry 'help'\n");
	tokErr = true;
}

static char *readLine() {
	char *line;
	size_t i, z;
	int c;

	i = 0;
	z = 20;
	line = malloc(z * sizeof(char));

	do {
		c = getch();
		/*
		 * '\b':	backspace
		 * '\177':	DEL
		 * '\25':	Ctrl+U (Scan code: 0x15) 
		 * '\30':	Ctrl+X (Scan code: 0x18)
		 *
		 * See Keyboard codes for more.
		 */
		if (strchr("\b\177\25\30", c) != NULL) {
			do {
				if (i == 0)
				  break;
				/*
				 * The first '\b': move cursor backward 1 character.
				 * The second ' ': output a whitespace to replace the original character
				 *				   and move cursor forward 1 char.
				 * The third '\b': move cursor backward 1 character.
				 *				   Now the cursor is under the whitespace and it looks good.
				 */
				printf("\b \b");
				--i;
			} while (c == '\25' || c == '\30');
		} else if (c < ' ' && c != '\n') {
			/* Send a bell */
			putch('\7');
		} else {
			putch(c);
			line[i++] = c;
			if (i == z) {
				z *= 2;
				line = realloc(line, z * sizeof(char));
			}
		}
	} while (c != '\n');
	line[i] = 0;
	return line;
}

bool runTrailer() {
/* Run the trailer function between loading Minix and handing control to it.
 * Return true iff there was no error.
 */
	Token *savedCmds = cmds;

	cmds = NULL;
	tokenize(&cmds, "trailer");
	while (cmds != NULL) {
		execute();
	}
	cmds = savedCmds;
	return !tokErr;
}

static void monitor() {
	char *line;
	
	unschedule();
	tokErr = false;
	printf("%s>", bootDev.name);
	line = readLine();
	tokenize(&cmds, line);
	free(line);
	escape();		/* Reset esc flag if ESC was pressed. */
}

void boot() {
	determineAvailableMemory();
	initialize();
	getParameters();
	
	while (true) {
		while (cmds != NULL) {
			execute();
		}
		monitor();
	}

	if(false)
	  printTokens();
}

