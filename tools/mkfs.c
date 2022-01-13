#include <time.h>
#include "common.h"
#include "util.h"
#include "mkfs.h"

#define NULL			((void *)0)
#define KB				1024
#define BIN				2
#define BINGRP			2
#define CACHE_SIZE		20
#define UMAP_SIZE		(ULONG_MAX / MAX_BLOCK_SIZE)		
#define BLOCK_OFF(n)	(OFFSET(lba) + (n) * blockSize)
#define ZONE_SHIFT		0
#define INODE_MAP		2
#define MAX_MAX_SIZE	((unsigned long) 0xffffffff)

typedef struct {
	char blockBuf[MAX_BLOCK_SIZE];
	block_t blockNum;
	bool dirty;
	int useCount;
} Cache;

static Cache cache[CACHE_SIZE];
static long currentTime;
static char *device;
static int deviceFd;
static uint32_t lba;
static unsigned int blockSize;
static int inodesPerBlock;
static char *zero;
static block_t numBlocks;
static unsigned int numInodes;
static char umap[UMAP_SIZE];
static int inodeOffset;
static int nextZone, nextInode, zoneOffset, zoneSize;
static zone_t zoneMap;

static void usage(char *s) {
	fprintf(stderr, "Usage: %s device\n", s);
	exit(1);
}

static char *allocBlock() {
	char *buf;

	if (!(buf = malloc(blockSize)))
	  errExit("Couldn't allocate file system buffer");
	memset(buf, 0, blockSize);

	return buf;
}

static block_t computeBlocks() {
	PartitionEntry pe;
	block_t blocks;

	getActivePartition(deviceFd, &pe);

	lba = pe.lowSector;		/* Save the lba for writing data to disk */
	blocks = pe.sectorCount * SECTOR_SIZE / blockSize;
	if (blocks < 5)
	  fatal("Block count too small");

	if (ULONG_MAX / blockSize <= blocks - 1) {
		fprintf(stderr, "Warning: too big for file system to currently\n"
						"run on (max 4GB), truncating.\n");
		blocks = ULONG_MAX / blockSize;
	}

	return blocks;
}

static ino_t computeInodes(block_t blocks) {
	ino_t inodes;
	int kb;

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

	if (inodes < 1)
	  fatal("Inode count too small");

	return inodes;
}

static void initCache() {
	Cache *bp;
	for (bp = cache; bp < &cache[CACHE_SIZE]; ++bp) {
		bp->blockNum = -1;
	}
}

static bool readAndSet(block_t n) {
	int idx, off, mask;
	bool r;

	idx = n / CHAR_BIT;
	off = n % CHAR_BIT;
	mask = 1 << off;
	r = umap[idx] & mask ? true : false;
	umap[off] |= mask;
	return r;
}

static void mxRead(block_t n, char *buf) {
	Lseek(device, deviceFd, BLOCK_OFF(n));
	Read(device, deviceFd, buf, blockSize);
}

static void mxWrite(block_t n, char *buf) {
	Lseek(device, deviceFd, BLOCK_OFF(n));
	Write(device, deviceFd, buf, blockSize);
}

typedef void (*CopyFunc)(char *cacheBuf, char *userBuf, size_t size); 
typedef void (*PostTransferFunc)(block_t n, char *userBuf, Cache *fp);

static void transferBlock(block_t n, char *buf, CopyFunc copyFunc, PostTransferFunc postFunc) {
	Cache *bp, *fp;

	/* Look for block in cache */
	fp = NULL;
	for (bp = cache; bp < &cache[CACHE_SIZE]; ++bp) {
		if (bp->blockNum == n) {
			copyFunc(bp->blockBuf, buf, blockSize);
			bp->dirty = true;
			return;
		}

		/* Remember clean block. */
		if (!bp->dirty) {
			if (!fp || fp->useCount > bp->useCount) {
			  fp = bp;
			}
		}
	}

	/* Block not in cache */
	if (!fp) {
		/* No clean buf, flush one. */
		for (bp = cache, fp = cache; bp < &cache[CACHE_SIZE]; ++bp) {
			if (fp->useCount > bp->useCount)
			  fp = bp;
		}
		mxWrite(fp->blockNum, fp->blockBuf);
	}

	postFunc(n, buf, fp);

	copyFunc(fp->blockBuf, buf, blockSize);
}

static void copyFromCache(char *cacheBuf, char *userBuf, size_t size) {
	memcpy(userBuf, cacheBuf, size);
}

static void postGetBlock(block_t n, char *userBuf, Cache *fp) {
	mxRead(n, fp->blockBuf);
	fp->dirty = false;
	fp->useCount = 0;
	fp->blockNum = n;
}

static void getBlock(block_t n, char *buf) {
	/* First access returns a zero block. */
	if (!readAndSet(n)) {
		memcpy(buf, zero, blockSize);
		return;
	}
	transferBlock(n, buf, copyFromCache, postGetBlock);
}

static void copyToCache(char *cacheBuf, char *userBuf, size_t size) {
	memcpy(cacheBuf, userBuf, size);
}

static void postPutBlock(block_t n, char *userBuf, Cache *fp) {
	fp->dirty = true;
	fp->useCount = 1;
	fp->blockNum = n;
}

static void putBlock(block_t n, char *buf) {
	readAndSet(n);
	transferBlock(n, buf, copyToCache, postPutBlock);	
}

static int bitmapSize(uint32_t numBits, int blkSize) {
	int blocks;

	blocks = numBits / FS_BITS_PER_BLOCK(blkSize);
	if (blocks * FS_BITS_PER_BLOCK(blkSize) < numBits)
	  ++blocks;
	return blocks;
}

static void insertBit(block_t n, int bit) {
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

static void initSuperBlock(zone_t zones, ino_t inodes) {
	int inodeBlocks, initBlocks;
	zone_t indirectZones, doubleIndirectZones;
	zone_t totalZones;
	char *buf;
	SuperBlock *sup;
	int i;

	buf = allocBlock();
	sup = (SuperBlock *) buf;

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
	zoneSize = 1 << ZONE_SHIFT;		

	/* Write super block. */
	putBlock(1, buf);
	
	/* Clear maps and inodes. */
	for (i = INODE_MAP; i < initBlocks; ++i) {
		putBlock(i, zero);
	}

	nextZone = sup->s_first_data_zone;
	nextInode = 1;
	zoneMap = INODE_MAP + sup->s_imap_blocks;

	insertBit(zoneMap, 0);		/* bit zero must always be allocated */
	insertBit(INODE_MAP, 0);	/* inode zero not used but must be allocated */

	free(buf);
}

static ino_t allocInode(int mode, int uid, int gid) {
	ino_t num;
	block_t b;
	int off;
	INode *inp;
	INode *inodes;	/* an array of INode */

	num = nextInode++;
	if (num > numInodes) {
		fprintf(stderr, "have %d inodes\n", numInodes);
		fatal("File system does not have enough inodes");
	}
	b = ((num - 1) / inodesPerBlock) + inodeOffset;
	off = (num - 1) % inodesPerBlock;

	if (! (inodes = malloc(INODES_PER_BLOCK(blockSize) * INODE_SIZE)))
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

static zone_t allocZone() {
	zone_t z;
	block_t b;
	int i;

	z = nextZone++;
	b = z << ZONE_SHIFT;
	if (b + zoneSize > numBlocks)
	  fatal("File system not big enough for all the files");
	for (i = 0; i < zoneSize; ++i) {
		putBlock(b + i, zero);
	}
	insertBit(zoneMap, z - zoneOffset);

	return z;
}

static void addZone(ino_t n, zone_t z, size_t bytes, time_t currTime) {
	block_t b;
	int off, i;
	zone_t indirZone;
	zone_t *zones;	/* an array of zone_t */
	INode *inp;		
	INode *inodes;	/* an array of INode */

	if (! (zones = malloc(INDIRECT_ZONES(blockSize) * ZONE_NUM_SIZE)))
	  errExit("couldn't allocate indirect block");

	if (! (inodes = malloc(INODES_PER_BLOCK(blockSize) * INODE_SIZE)))
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

static void initRootDir(ino_t rootInum) {
	zone_t z;

	z = allocZone();
	addZone(rootInum, z, 2 * sizeof(Directory), currentTime);
}

static void flush() {
	Cache *bp;
	
	for (bp = cache; bp < &cache[CACHE_SIZE]; ++bp) {
		if (bp->dirty) {
			mxWrite(bp->blockNum, bp->blockBuf);
			bp->dirty = false;
		}
	}
}

int main(int argc, char *argv[]) {
	zone_t zones;
	int mode, uid, gid;
	ino_t rootInum;


	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usage(argv[0]);

	currentTime = time(NULL);
	device = argv[1];
	deviceFd = RWOpen(device);
	blockSize = MAX_BLOCK_SIZE;	
	inodesPerBlock = INODES_PER_BLOCK(blockSize);
	zero = allocBlock();

	numBlocks = computeBlocks();
	numInodes = computeInodes(numBlocks);

	mode = 040777;
	uid = BIN;
	gid	= BINGRP;

	initCache();
	
	/* Write a null boot block. */
	putBlock(0, zero);

	zones = numBlocks >> ZONE_SHIFT;

	initSuperBlock(zones, numInodes);
	rootInum = allocInode(mode, uid, gid);
	initRootDir(rootInum);

	flush();

	Close(device, deviceFd);

	return EXIT_SUCCESS;
}


