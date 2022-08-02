#include <time.h>
#include "common.h"

#include "../include/minix/dmap_nr.h"
#include "../include/limits.h"
#include "../include/dirent.h"
#include "../include/minix/const.h"
#include "../servers/fs/const.h"
#include "../servers/fs/type.h"
#include "../servers/fs/super.h"

#define UMAP_SIZE		(ULONG_MAX / MAX_BLOCK_SIZE)		
#define BLOCK_OFF(n)	(OFFSET(lowSector) + (n) * blockSize)
#define SUPER_OFFSET	BLOCK_OFF(0) + SUPER_OFFSET_BYTES
#define MAX_MAX_SIZE	((unsigned long) 0xffffffff)
#define ZONE_SHIFT		0

#define	ROOT_GRP		0
#define BIN				2
#define BIN_GRP			2
#define TTY_GRP			4
#define RDEV(major, minor)	( (((major) & BYTE) << MAJOR) | \
								(((minor) & BYTE) << MINOR) )

static bool init;
static uint32_t reservedSectors = 0;
static uint32_t sectorCount;
static long currentTime;
static char *device;
static int deviceFd;
static uint32_t lowSector;
static unsigned int blockSize;
static unsigned inodesPerBlock;
static char *zero;
static Block_t numBlocks;
static unsigned int numInodes;
static char umap[UMAP_SIZE];
static int inodeOffset;
static int nextZone, nextInode, zoneOffset, blocksPerZone;
static Zone_t zoneMap;
static char modeString[11] = {0};
static char timeBuf[26];

static char *progName;
static char *units[] = {"bytes", "KB", "MB", "GB"};

static void usage() {
	usageErr("%s [-b blocks] [-i inodes] [-B blocksize] device\n", progName);
}

static char *allocBlock() {
	char *buf;

	if (!(buf = malloc(blockSize)))
	  errExit("Couldn't allocate file system buffer");
	memset(buf, 0, blockSize);

	return buf;
}

static Block_t sizeup() {
	Block_t maxBlocks;

	maxBlocks = sectorCount / RATIO(blockSize);

	/* Reserve space for the boot image */
	maxBlocks -= reservedSectors / RATIO(blockSize);

	return maxBlocks;
}

static Block_t computeBlocks(Block_t blocks) {
	Block_t maxBlocks;
	
	maxBlocks = sizeup();
	if (blocks == 0) {
		blocks = maxBlocks;
		if (blocks < 1)
		  fatal("%s: this device can't hold a filesystem.\n", progName);
	}
	if (blocks > maxBlocks) 
	  fatal("%s: number of blocks too large for device.\n", progName);

	if (blocks < 5)
	  fatal("Block count too small");

	if (ULONG_MAX / blockSize <= blocks - 1) {
		fprintf(stderr, "Warning: too big for file system to currently\n"
						"run on (max 4GB), truncating.\n");
		blocks = ULONG_MAX / blockSize;
	}

	return blocks;
}

static Ino_t computeInodes(Block_t blocks, Ino_t inodes) {
	int kb;

	if (inodes == 0) {
		kb = blocks * blockSize / KB;
		/* We assume the default average file size is 2KB, but may 
		 * become larger when the disk size is above 20M, so reduce 
		 * the default number of inodes.
		 */
		if (kb >= 100000)
		  inodes = kb / 7;
		else if (kb >= 80000)
		  inodes = kb / 6;
		else if (kb >= 60000)
		  inodes = kb / 5;
		else if (kb >= 40000)
		  inodes = kb / 4;
		else if (kb >= 20000)
		  inodes = kb / 3;
		else
		  inodes = kb / 2;

		/* Round up to fill inode block */
		inodes = (inodes + inodesPerBlock - 1) / inodesPerBlock * inodesPerBlock;
	}

	if (inodes < 1)
	  fatal("Inode count too small");

	return inodes;
}

static bool readAndSet(Block_t n) {
	int idx, off, mask;
	bool r;

	idx = n / CHAR_BIT;
	off = n % CHAR_BIT;
	mask = 1 << off;
	r = umap[idx] & mask ? true : false;
	umap[idx] |= mask;
	return r;
}

static void getBlock(Block_t n, char *buf) {
	/* First access returns a zero block. */
	if (!readAndSet(n)) {
		memcpy(buf, zero, blockSize);
		return;
	}
	Lseek(device, deviceFd, BLOCK_OFF(n));
	Read(device, deviceFd, buf, blockSize);
}

static void putBlock(Block_t n, char *buf) {
	readAndSet(n);
	Lseek(device, deviceFd, BLOCK_OFF(n));
	Write(device, deviceFd, buf, blockSize);
}

static int bitmapSize(uint32_t numBits, int blkSize) {
	int blocks;

	blocks = numBits / FS_BITS_PER_BLOCK(blkSize);
	if (blocks * FS_BITS_PER_BLOCK(blkSize) < numBits)
	  ++blocks;
	return blocks;
}

static void insertBit(Block_t n, int bit) {
	int idx, off;
	short *buf;		/* an array of short */

	if (n < 0)
	  fatal("insertBit called with negative argument");

	buf = (short *) allocBlock();
	getBlock(n, (char *) buf);
	idx = bit / FS_BITCHUNK_BITS;
	off = bit % FS_BITCHUNK_BITS;
	buf[idx] |= 1 << off;
	putBlock(n, (char *) buf);

	free(buf);
}

static void initSuperBlock(Zone_t zones, Ino_t inodes) {
	int inodeBlocks, initBlocks;
	Zone_t indirectZones, doubleIndirectZones;
	Zone_t totalZones;
	SuperBlock *sup;
	int i;

	sup = (SuperBlock *) allocBlock();
	sup->s_inodes = inodes;
	sup->s_zones = zones;
	sup->s_imap_blocks = bitmapSize(inodes + 1, blockSize);
	sup->s_zmap_blocks = bitmapSize(zones, blockSize);
	inodeOffset = IMAP_OFFSET + sup->s_imap_blocks + sup->s_zmap_blocks;
	inodeBlocks = (inodes + inodesPerBlock - 1) / inodesPerBlock;
	initBlocks = inodeOffset + inodeBlocks;

	sup->s_first_data_zone = (initBlocks + (1 << ZONE_SHIFT) - 1) >> ZONE_SHIFT;
	zoneOffset = sup->s_first_data_zone - 1;
	sup->s_log_zone_size = ZONE_SHIFT;
	sup->s_magic = SUPER_V3;
	sup->s_block_size = blockSize;
	sup->s_disk_version = 0;

	indirectZones = INDIRECT_ZONES(blockSize);
	doubleIndirectZones = indirectZones * indirectZones;
	totalZones = NR_DIRECT_ZONES + indirectZones + doubleIndirectZones;
	sup->s_max_size = (MAX_MAX_SIZE / blockSize < totalZones) ? 
						MAX_MAX_SIZE : totalZones * blockSize;

	/* number of blocks per zone */
	blocksPerZone = 1 << ZONE_SHIFT;		

	/* Write super block. */
	Lseek(device, deviceFd, SUPER_OFFSET);
	Write(device, deviceFd, (char *) sup, SUPER_BLOCK_BYTES);
	
	/* Clear maps and inodes. */
	for (i = IMAP_OFFSET; i < initBlocks; ++i) {
		putBlock(i, zero);
	}

	nextZone = sup->s_first_data_zone;
	nextInode = 1;
	zoneMap = IMAP_OFFSET + sup->s_imap_blocks;

	insertBit(zoneMap, 0);		/* bit zero must always be allocated */
	insertBit(IMAP_OFFSET, 0);	/* inode zero not used but must be allocated */

	free(sup);
}

static Ino_t allocInode(int mode, int uid, int gid, Zone_t z0) {
	Ino_t num;
	Block_t b;
	int off;
	DiskInode *dip;
	DiskInode *inodes;	/* an array of DiskInode */

	num = nextInode++;
	if (num > numInodes) {
		fprintf(stderr, "have %d inodes\n", numInodes);
		fatal("File system does not have enough inodes");
	}
	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate a block of inodes");

	b = ((num - 1) / inodesPerBlock) + inodeOffset;
	off = (num - 1) % inodesPerBlock;

	getBlock(b, (char *) inodes);
	dip = &inodes[off];
	dip->d_mode = mode;
	dip->d_uid = uid;
	dip->d_gid = gid;
	dip->d_zone[0] = z0;
	dip->d_atime = 
	dip->d_mtime =
	dip->d_ctime = currentTime;
	putBlock(b, (char *) inodes);

	free(inodes);

	/* This assumes the bit is in the first inode map block. */
	insertBit(IMAP_OFFSET, num);
	return num;
}

static Zone_t allocZone() {
	Zone_t z;
	Block_t b;
	int i;

	z = nextZone++;
	b = z << ZONE_SHIFT;
	if (b + blocksPerZone > numBlocks)
	  fatal("File system not big enough for all the files");
	for (i = 0; i < blocksPerZone; ++i) {
		putBlock(b + i, zero);
	}
	insertBit(zoneMap, z - zoneOffset);

	return z;
}

void addZone(Ino_t n, Zone_t z, size_t bytes, long currTime) {
	Block_t b;
	int off, i;
	Zone_t indirZone;
	Zone_t *zones;	/* an array of Zone_t */
	DiskInode *dip;		
	DiskInode *inodes;	/* an array of DiskInode */

	if (! (zones = malloc(blockSize)))
	  errExit("couldn't allocate indirect block");

	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate block of inodes");

	b = ((n - 1) / inodesPerBlock) + inodeOffset;
	off = (n - 1) % inodesPerBlock;
	getBlock(b, (char *) inodes);
	dip = &inodes[off];
	dip->d_size += bytes;
	dip->d_mtime = currTime;
	for (i = 0; i < NR_DIRECT_ZONES; ++i) {
		if (dip->d_zone[i] == 0) {
			dip->d_zone[i] = z;
			putBlock(b, (char *) inodes);
			free(zones);
			free(inodes);
			return;
		}
	}

	/* File has grown beyond a small file. */
	if (dip->d_zone[NR_DIRECT_ZONES] == 0) {
		dip->d_zone[NR_DIRECT_ZONES] = allocZone();
		putBlock(b, (char *) inodes);
	}
	indirZone = dip->d_zone[NR_DIRECT_ZONES];
	b = indirZone << ZONE_SHIFT;
	getBlock(b, (char *) zones);
	for (i = 0; i < INDIRECT_ZONES(blockSize); ++i) {
		if (zones[i] == 0) {
			zones[i] = z;
			putBlock(b, (char *) zones);
			free(zones);
			free(inodes);
			return;
		}
	}
	fatal("File has grown beyond single indirect");
}

static void enterDir(Ino_t parent, char *name, Ino_t child) {
	int k, l, i;
	Zone_t z;
	Block_t b, zoneBlocks;
	int off;
	DirEntry *dirs;		/* an array of DirEntry */
	DiskInode *inodes;		/* an array of DiskInode */
	DiskInode *dip;

	b = ((parent - 1) / inodesPerBlock) + inodeOffset;
	off = (parent - 1) % inodesPerBlock;
	
	if (! (dirs = malloc(blockSize)))
	  fatal("couldn't allocate directory entry");

	if (! (inodes = malloc(blockSize)))
	  fatal("couldn't allocate block of inodes entry");

	getBlock(b, (char *) inodes);
	dip = &inodes[off];
	dip->d_size += DIR_ENTRY_SIZE;
	dip->d_mtime = currentTime;
	for (k = 0; k < NR_DIRECT_ZONES; ++k) {
		z = dip->d_zone[k];
		if (z == 0) {
			z = allocZone();
			dip->d_zone[k] = z;
		}

		zoneBlocks = z << ZONE_SHIFT;
		for (l = 0; l < blocksPerZone; ++l) {
			getBlock(zoneBlocks + l, (char *) dirs);
			for (i = 0; i < NR_DIR_ENTRIES(blockSize); ++i) {
				if (dirs[i].d_ino == 0) {
					dirs[i].d_ino = child;
					memcpy(dirs[i].d_name, name, 
							min(strlen(name), DIR_SIZE));
					putBlock(b, (char *) inodes);
					putBlock(zoneBlocks + l, (char *) dirs);
					free(dirs);
					free(inodes);
					return;
				}
			}
		}
	}
	fprintf(stderr, 
		"Directory-inode %u beyond direct blocks.  Could not enter %s\n", 
		parent, name);
	fatal("Halt");
}

static void increaseLink(Ino_t n) {
	Block_t b;
	int off;
	DiskInode *inodes;	

	b = ((n - 1) / inodesPerBlock) + inodeOffset;
	off = (n - 1) % inodesPerBlock;
	
	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate a block of inodes");

	getBlock(b, (char *) inodes);
	++inodes[off].d_nlinks;
	putBlock(b, (char *) inodes);

	free(inodes);
}

static void printSize(char *format, size_t size) {
	int i = 0;

	if (size == ULONG_MAX) {
	    size = 4;
		i = 3;
	} else {
		while (size >= KB) {
			size = size >> KB_SHIFT;
			++i;
		}
	}
	printf(format, size, units[i]);
}

static void printSuperBlock(SuperBlock *sup) {
	memset((char *) sup, 0, blockSize);
	Lseek(device, deviceFd, SUPER_OFFSET);
	Read(device, deviceFd, (char *) sup, SUPER_BLOCK_BYTES);

	printf("\nSuperBlock:\n");
	printf("  magic: 0x%x\n", sup->s_magic);
	printSize("  inode size: %d %s\n", INODE_SIZE);
	printSize("  block size: %d %s\n", sup->s_block_size);
	printSize("  max file size: %d %s\n", sup->s_max_size);

	printf("\n");
	printf("  inodes: %u\n", sup->s_inodes);
	printf("  zones: %u\n", sup->s_zones);
	printf("  blocks: %u\n", sup->s_zones << ZONE_SHIFT);

	printf("\n");
	printf("  inodes per block: %d\n", inodesPerBlock);
	printf("  blocks per zone: %u\n", 1 << sup->s_log_zone_size);

	printf("\n");
	printf("  start offset blocks: %d\n", IMAP_OFFSET);
	printf("  inode-map blocks: %u\n", sup->s_imap_blocks);
	printf("  zone-map blocks: %u\n", sup->s_zmap_blocks);
	printf("  inode blocks: %u\n", (sup->s_inodes + inodesPerBlock - 1) / inodesPerBlock);
	printf("  first data zone: %u\n", sup->s_first_data_zone);
}

static void printMap(uint16_t mapBlocks, uint16_t startBlock) {
#define COLS	8
	static const char *ellipsis = "......";
	int j, k, row, mapsCnt;
	short *maps;
	bool print, empty;
	char *buf;

	if (! (buf = malloc(blockSize)))
	  errExit("couldn't allocate a block of maps");

	mapsCnt = blockSize / sizeof(short);
	for (k = 0; k < mapBlocks; ++k) {
		getBlock(startBlock + k, buf);
		maps = (short *) buf;
		row = 1;
		empty = false;
		while (mapsCnt > 0) {
			print = false;
			/* determine if this row is printable */
			for (j = 0; j < COLS && j < mapsCnt; ++j) {
				if (maps[j] != 0) {
					print = true;
					break;
				}
			}
			if (mapsCnt <= COLS && empty) {	/* if last row and empty before */
				printf("        %s\n", ellipsis);
			}
			if (print) {	/* if printable */
				empty = false;

				printf("  [%03d] ", row);
				for (j = 0; j < COLS && j < mapsCnt; ++j) {
					printf("%06o ", maps[j]);
				}
				printf("\n");
			} else {
				if (mapsCnt <= COLS) /* if last row and empty */
				  printf("  [%03d] %s\n", row, ellipsis);

				empty = true;
			}
			maps += COLS;
			mapsCnt -= COLS;
			++row;
		}
	}

	free(buf);
}

static char *getModeString(Mode_t mode) {
	int i;
	Mode_t fileType;

	memset(modeString, '-', 10);
	fileType = mode & I_TYPE;

	i = 0;
	/* file type */
	if (fileType == I_BLOCK_SPECIAL)
	  modeString[i] = 'b';
	else if (fileType == I_DIRECTORY)
	  modeString[i] = 'd';
	else if (fileType == I_CHAR_SPECIAL)
	  modeString[i] = 'c';
	//TODO others


	/* owner bits */
	++i;
	if (mode & S_IRUSR)
	  modeString[i] = 'r';
	++i;
	if (mode & S_IWUSR)
	  modeString[i] = 'w';
	++i;
	if (mode & S_IXUSR)
	  modeString[i] = 'x';

	/* group bits */
	++i;
	if (mode & S_IRGRP)
	  modeString[i] = 'r';
	++i;
	if (mode & S_IWGRP)
	  modeString[i] = 'w';
	++i;
	if (mode & S_IXGRP)
	  modeString[i] = 'x';

	/* other bits */
	++i;
	if (mode & S_IROTH)
	  modeString[i] = 'r';
	++i;
	if (mode & S_IWOTH)
	  modeString[i] = 'w';
	++i;
	if (mode & S_IXOTH)
	  modeString[i] = 'x';
	
	return modeString;
}

static char *getTimeString(Time_t t) {
	struct tm *ct = localtime((time_t *) &t);
	strftime(timeBuf, 26, "%Y-%m-%d %H:%M:%S", ct);
	return timeBuf;
}

static void printInode(DiskInode *inodes, DirEntry *dirs, int inoOff, Ino_t inoNum) {
	int insPerBlock;
	Block_t b;
	DiskInode *dip;
	int off, i;
	Mode_t fileType;

	insPerBlock = INODES_PER_BLOCK(blockSize);
	b = ((inoNum - 1) / insPerBlock) + inoOff;
	off = (inoNum - 1) % insPerBlock;
	getBlock(b, (char *) inodes);
	dip = &inodes[off];
	fileType = dip->d_mode & I_TYPE;

	printf("  %2u: ", inoNum);
	printf("mode=%07o, \"%s\", ", dip->d_mode, getModeString(dip->d_mode));
	printf("uid=%d, gid=%d, ", dip->d_uid, dip->d_gid);
	printf("nlink=%d, ", dip->d_nlinks);
	if (fileType == I_CHAR_SPECIAL || fileType == I_BLOCK_SPECIAL)
	  printf("rdev=0x%x, ", dip->d_zone[0]);
	else 
	  printf("zone[0]=%u, ", dip->d_zone[0]);
	printf("size=%u, ", dip->d_size);
	printf("mtime=%s\n", getTimeString(dip->d_mtime));

	if (fileType == I_DIRECTORY) {
		getBlock(dip->d_zone[0], (char *) dirs);
		for (i = 0; i < NR_DIR_ENTRIES(blockSize); ++i) {
			if (dirs[i].d_ino) {
				printf("     %8s (ino: %ld)\n", dirs[i].d_name, dirs[i].d_ino);
			}
		}
	}
}

static void printInodes(SuperBlock *sup) {
	DiskInode *inodes;
	DirEntry *dirs;
	unsigned short *maps, map;
	uint32_t inoOff;
	Ino_t inoNum;
	int k, j, i, mapsCnt;

	if (! (maps = malloc(blockSize)))
	  errExit("couldn't allocate a block of maps");

	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate a block of inodes");

	if (! (dirs = malloc(blockSize)))
	  errExit("couldn't allocate a block of directories");
	
	mapsCnt = blockSize / FS_BITCHUNK_BITS;
	inoOff = IMAP_OFFSET + sup->s_imap_blocks + sup->s_zmap_blocks;
	for (k = 0; k < sup->s_imap_blocks; ++k) {
		getBlock(IMAP_OFFSET + k, (char *) maps);
		for (j = 0; j < mapsCnt; ++j) {
			map = maps[j];
			for (i = 0; i < FS_BITCHUNK_BITS && map; ++i, map = map >> 1) {
				if (map & 0x1) {
					inoNum = k * FS_BITS_PER_BLOCK(blockSize) + j * FS_BITCHUNK_BITS + i;
					if (inoNum > 0)
					  printInode(inodes, dirs, inoOff, inoNum);
				}
			}
		}
	}

	free(maps);
	free(inodes);
	free(dirs);
}

static void printFS() {
	SuperBlock *sup;

	if (! (sup = malloc(blockSize)))
	  errExit("couldn't allocate a block of super block");

	printSuperBlock(sup);

	printf("\n\nInode-Map:\n");
	printMap(sup->s_imap_blocks, IMAP_OFFSET);

	printf("\nZone-Map:\n");
	printMap(sup->s_zmap_blocks, zoneMap);

	printf("\n\nInodes:\n");
	printInodes(sup);

	printf("\n");

	free(sup);
}

static void calibrateBlockSize() {
	if (!blockSize)
	  blockSize = MAX_BLOCK_SIZE;	
	if (blockSize % SECTOR_SIZE || blockSize < MIN_BLOCK_SIZE)
	  fatal("block size must be multiple of sector (%d) "
			"and at least %d bytes\n", 
			SECTOR_SIZE, MIN_BLOCK_SIZE);
	if (blockSize % INODE_SIZE)
	  fatal("block size must be multiple of inode size (%d bytes)\n",
				INODE_SIZE);
}

static Ino_t doMkdir(Ino_t pINum, char *dirName, mode_t mode, 
			int uid, int gid, Zone_t z0) {
	Ino_t iNum;

	iNum = allocInode(mode, uid, gid, z0);
	if (dirName == NULL)
	  pINum = iNum;

	enterDir(iNum, ".", iNum);
	enterDir(iNum, "..", pINum);
	increaseLink(iNum);
	increaseLink(pINum);

	if (dirName) 
	  enterDir(pINum, dirName, iNum);

	return iNum;
}

typedef struct {
	char *name;
	Mode_t mode;
	int uid;
	int gid;
	Dev_t rdev;
} DevFile;

static DevFile devFiles[] = {
	{ "console", I_CHAR_SPECIAL | 0620, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x0)    },
	{ "pty0",    I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0xC0) },
	{ "pty1",    I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0xC1) },
	{ "pty2",    I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0xC2) },
	{ "pty3",    I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0xC3) },
	{ "tty",     I_CHAR_SPECIAL | 0666, SUPER_USER, ROOT_GRP,  RDEV(CTTY_MAJOR, 0x0)   },
	{ "tty00",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x10) },
	{ "tty01",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x11) },
	{ "tty02",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x12) },
	{ "tty03",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x13) },
	{ "ttyc1",   I_CHAR_SPECIAL | 0600, SUPER_USER, ROOT_GRP,  RDEV(TTY_MAJOR, 0x1) },
	{ "ttyc2",   I_CHAR_SPECIAL | 0600, SUPER_USER, ROOT_GRP,  RDEV(TTY_MAJOR, 0x2) },
	{ "ttyc3",   I_CHAR_SPECIAL | 0600, SUPER_USER, ROOT_GRP,  RDEV(TTY_MAJOR, 0x3) },
	{ "ttyp0",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x80) },
	{ "ttyp1",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x81) },
	{ "ttyp2",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x82) },
	{ "ttyp3",   I_CHAR_SPECIAL | 0666, SUPER_USER, TTY_GRP,   RDEV(TTY_MAJOR, 0x83) }
};

static void mkSpecialFile(Ino_t pINum, char *name, DevFile *dfp) {
	Ino_t iNum;

	iNum = allocInode(dfp->mode, dfp->uid, dfp->gid, dfp->rdev);
	increaseLink(iNum);
	enterDir(pINum, dfp->name, iNum);
}

static void initFS(Ino_t pINum) {
	Mode_t mode;
	int uid, gid;
	Ino_t iNum;
	DevFile *dfp;
	
	mode = I_DIRECTORY | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; 
	uid = gid = 0;

	iNum = doMkdir(pINum, "dev", mode, uid, gid, 0);
	
	for (dfp = &devFiles[0]; dfp < arrayLimit(devFiles); ++dfp) {
		mkSpecialFile(iNum, dfp->name, dfp);
	}
}

int main(int argc, char *argv[]) {
	PartitionEntry pe;
	Zone_t zones;
	Block_t blocks;
	Ino_t inodes;
	int mode, uid, gid;
	Ino_t rootInoNum;
	int ch;

	progName = argv[0];
	blocks = 0;
	inodes = 0;
	while ((ch = getopt(argc, argv, "b:i:B:r:ch")) != EOF) {
		switch (ch) {
			case 'b':
				blocks = strtoul(optarg, (char **) NULL, 0);
				break;
			case 'i':
				inodes = strtoul(optarg, (char **) NULL, 0);
				break;
			case 'B':
				blockSize = atoi(optarg);
				break;
			case 'r':
				reservedSectors = atoi(optarg);
				break;
			case 'c':
				init = true;
				break;	
			default:
				usage();
		}
	}
	if (argc - optind < 1)
	  usage();

	device = parseDevice(argv[optind], NULL, &pe);
	lowSector = pe.lowSector;
	sectorCount = pe.sectorCount;
	deviceFd = RWOpen(device);
	currentTime = time(NULL);

	calibrateBlockSize();
	inodesPerBlock = INODES_PER_BLOCK(blockSize);
	blocks = computeBlocks(blocks);
    inodes = computeInodes(blocks, inodes);

	numBlocks = blocks;
	numInodes = inodes;

	zero = allocBlock();
	putBlock(BOOT_BLOCK, zero);		/* Write a null boot block. */

	zones = numBlocks >> ZONE_SHIFT;
	initSuperBlock(zones, numInodes);

	mode = I_DIRECTORY | RWX_MODES;
	uid = BIN;
	gid	= BIN_GRP;
	rootInoNum = doMkdir(0, NULL, mode, uid, gid, 0);
	if (init) 
	  initFS(rootInoNum);

	printFS();

	Close(device, deviceFd);

	return EXIT_SUCCESS;
}


