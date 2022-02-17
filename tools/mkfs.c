#include <time.h>
#include "common.h"
#include "util.h"

#include "../include/limits.h"
#include "../include/dirent.h"
#include "../include/minix/const.h"
#include "../servers/fs/const.h"
#include "../servers/fs/type.h"
#include "../servers/fs/super.h"

#define NULL			((void *)0)
#define KB				1024
#define KB_SHIFT		10
#define BIN				2
#define BINGRP			2
#define CACHE_SIZE		20
#define UMAP_SIZE		(ULONG_MAX / MAX_BLOCK_SIZE)		
#define BLOCK_OFF(n)	(OFFSET(lowSector) + (n) * blockSize)
#define SUPER_OFFSET	BLOCK_OFF(0) + SUPER_OFFSET_BYTES
#define INODE_MAP		2		/* Inode-map offset blocks */
#define MAX_MAX_SIZE	((unsigned long) 0xffffffff)
#define ZONE_SHIFT				0

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

static char *procName;
static char *units[] = {"bytes", "KB", "MB", "GB"};

static void usage() {
	usageErr("%s [-b blocks] [-i inodes] [-B blocksize] device\n", procName);
}

static char *allocBlock() {
	char *buf;

	if (!(buf = malloc(blockSize)))
	  errExit("Couldn't allocate file system buffer");
	memset(buf, 0, blockSize);

	return buf;
}

static Block_t sizeup() {
	PartitionEntry pe;

	getActivePartition(deviceFd, &pe);
	lowSector = pe.lowSector;	/* Save the lowSector for writing data to disk */
	return pe.sectorCount * (blockSize / SECTOR_SIZE);
}

static Block_t computeBlocks(Block_t blocks) {
	Block_t maxBlocks;
	
	maxBlocks = sizeup();
	if (blocks == 0) {
		blocks = maxBlocks;
		if (blocks < 1)
		  fatal("%s: this device can't hold a filesystem.\n", procName);
	}
	if (blocks > maxBlocks) 
	  fatal("%s: number of blocks too large for device.\n", procName);

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
	inodeOffset = INODE_MAP + sup->s_imap_blocks + sup->s_zmap_blocks;
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
	for (i = INODE_MAP; i < initBlocks; ++i) {
		putBlock(i, zero);
	}

	nextZone = sup->s_first_data_zone;
	nextInode = 1;
	zoneMap = INODE_MAP + sup->s_imap_blocks;

	insertBit(zoneMap, 0);		/* bit zero must always be allocated */
	insertBit(INODE_MAP, 0);	/* inode zero not used but must be allocated */

	free(sup);
}

static Ino_t allocInode(int mode, int uid, int gid) {
	Ino_t num;
	Block_t b;
	int off;
	DiskInode *inp;
	DiskInode *inodes;	/* an array of DiskInode */

	num = nextInode++;
	if (num > numInodes) {
		fprintf(stderr, "have %d inodes\n", numInodes);
		fatal("File system does not have enough inodes");
	}
	b = ((num - 1) / inodesPerBlock) + inodeOffset;
	off = (num - 1) % inodesPerBlock;

	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate a block of inodes");

	getBlock(b, (char *) inodes);
	inp = &inodes[off];
	inp->mode = mode;
	inp->uid = uid;
	inp->gid = gid;
	putBlock(b, (char *) inodes);

	free(inodes);

	/* This assumes the bit is in the first inode map block. */
	insertBit(INODE_MAP, num);
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

static void addZone(Ino_t n, Zone_t z, size_t bytes, long currTime) {
	Block_t b;
	int off, i;
	Zone_t indirZone;
	Zone_t *zones;	/* an array of Zone_t */
	DiskInode *inp;		
	DiskInode *inodes;	/* an array of DiskInode */

	if (! (zones = malloc(blockSize)))
	  errExit("couldn't allocate indirect block");

	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate block of inodes");

	b = ((n - 1) / inodesPerBlock) + inodeOffset;
	off = (n - 1) % inodesPerBlock;
	getBlock(b, (char *) inodes);
	inp = &inodes[off];
	inp->size += bytes;
	inp->mtime = currTime;
	for (i = 0; i < NR_DIRECT_ZONES; ++i) {
		if (inp->zones[i] == 0) {
			inp->zones[i] = z;
			putBlock(b, (char *) inodes);
			free(zones);
			free(inodes);
			return;
		}
	}

	/* File has grown beyond a small file. */
	if (inp->zones[NR_DIRECT_ZONES] == 0) {
		inp->zones[NR_DIRECT_ZONES] = allocZone();
		putBlock(b, (char *) inodes);
	}
	indirZone = inp->zones[NR_DIRECT_ZONES];
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

static void createDir(Ino_t parent, char *name, Ino_t child) {
	int k, l, i;
	Zone_t z;
	Block_t b, zoneBlocks;
	int off;
	DirEntry *dirs;	/* an array of Directory */
	DiskInode *inodes;		/* an array of DiskInode */

	b = ((parent - 1) / inodesPerBlock) + inodeOffset;
	off = (parent - 1) % inodesPerBlock;
	
	if (! (dirs = malloc(blockSize)))
	  fatal("couldn't allocate directory entry");

	if (! (inodes = malloc(blockSize)))
	  fatal("couldn't allocate block of inodes entry");

	getBlock(b, (char *) inodes);
	for (k = 0; k < NR_DIRECT_ZONES; ++k) {
		z = inodes[off].zones[k];
		if (z == 0) {
			z = allocZone();
			inodes[off].zones[k] = z;
			putBlock(b, (char *) inodes);
		}
		zoneBlocks = z << ZONE_SHIFT;
		for (l = 0; l < blocksPerZone; ++l) {
			getBlock(zoneBlocks + l, (char *) dirs);
			for (i = 0; i < NR_DIR_ENTRIES(blockSize); ++i) {
				if (dirs[i].d_ino == 0) {
					dirs[i].d_ino = child;
					memcpy(dirs[i].d_name, name, 
							min(strlen(name), DIR_SIZE));
					putBlock(zoneBlocks + l, (char *) dirs);
					free(dirs);
					free(inodes);
					return;
				}
			}
		}
	}
	fprintf(stderr, 
		"Directory-inode %lu beyond direct blocks.  Could not enter %s\n", 
		parent, name);
	fatal("Halt");
}

static void increseLink(Ino_t n, int linkCount) {
	Block_t b;
	int off;
	DiskInode *inodes;	

	b = ((n - 1) / inodesPerBlock) + inodeOffset;
	off = (n - 1) % inodesPerBlock;
	
	if (! (inodes = malloc(blockSize)))
	  errExit("couldn't allocate a block of inodes");

	getBlock(b, (char *) inodes);
	inodes[off].nlinks += linkCount;
	putBlock(b, (char *) inodes);

	free(inodes);
}

static void initRootDir(Ino_t inoNum) {
	Zone_t z;

	z = allocZone();
	addZone(inoNum, z, 2 * sizeof(DirEntry), currentTime);
	createDir(inoNum, ".", inoNum);
	createDir(inoNum, "..", inoNum);
	increseLink(inoNum, 2);
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
	printf("  inodes: %lu\n", sup->s_inodes);
	printf("  zones: %lu\n", sup->s_zones);
	printf("  blocks: %lu\n", sup->s_zones << ZONE_SHIFT);

	printf("\n");
	printf("  inodes per block: %d\n", inodesPerBlock);
	printf("  blocks per zone: %u\n", 1 << sup->s_log_zone_size);

	printf("\n");
	printf("  start offset blocks: %d\n", INODE_MAP);
	printf("  inode-map blocks: %u\n", sup->s_imap_blocks);
	printf("  zone-map blocks: %u\n", sup->s_zmap_blocks);
	printf("  inode blocks: %lu\n", (sup->s_inodes + inodesPerBlock - 1) / inodesPerBlock);
	printf("  first data zone: %lu\n", sup->s_first_data_zone);
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

static void printInode(DiskInode *inodes, DirEntry *dirs, int inoOff, Ino_t inoNum) {
	int insPerBlock;
	Block_t b;
	int off, i;

	insPerBlock = INODES_PER_BLOCK(blockSize);
	b = ((inoNum - 1) / insPerBlock) + inoOff;
	off = (inoNum - 1) % insPerBlock;
	getBlock(b, (char *) inodes);

	printf("  %lu: ", inoNum);
	printf("mode=%06o, ", inodes[off].mode);
	printf("uid=%d, gid=%d, size=%lu, ", inodes[off].uid, inodes[off].gid, inodes[off].size);
	printf("zone[0]=%lu\n", inodes[off].zones[0]);

	if ((inodes[off].mode & I_TYPE) == I_DIRECTORY) {
		getBlock(inodes[off].zones[0], (char *) dirs);
		for (i = 0; i < NR_DIR_ENTRIES(blockSize); ++i) {
			if (dirs[i].d_ino) {
				printf("     %s\n", dirs[i].d_name);
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
	inoOff = INODE_MAP + sup->s_imap_blocks + sup->s_zmap_blocks;
	for (k = 0; k < sup->s_imap_blocks; ++k) {
		getBlock(INODE_MAP + k, (char *) maps);
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
	printMap(sup->s_imap_blocks, INODE_MAP);

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

int main(int argc, char *argv[]) {
	Zone_t zones;
	Block_t blocks;
	Ino_t inodes;
	int mode, uid, gid;
	Ino_t rootInoNum;
	int ch;

	procName = argv[0];

	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usage();

	blocks = 0;
	inodes = 0;
	while ((ch = getopt(argc, argv, "b:i:B:")) != EOF) {
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
			default:
				usage();
		}
	}
	if (argc - optind < 1)
	  usage();

	device = argv[optind];
	deviceFd = RWOpen(device);
	currentTime = time(NULL);

	calibrateBlockSize();
	inodesPerBlock = INODES_PER_BLOCK(blockSize);
	blocks = computeBlocks(blocks);
    inodes = computeInodes(blocks, inodes);

	numBlocks = blocks;
	numInodes = inodes;
	mode = I_DIRECTORY | RWX_MODES;
	uid = BIN;
	gid	= BINGRP;

	zero = allocBlock();
	putBlock(BOOT_BLOCK, zero);		/* Write a null boot block. */

	zones = numBlocks >> ZONE_SHIFT;

	initSuperBlock(zones, numInodes);
	rootInoNum = allocInode(mode, uid, gid);
	initRootDir(rootInoNum);

	printFS();

	Close(device, deviceFd);

	return EXIT_SUCCESS;
}


