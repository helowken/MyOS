#include "../include/code.h"
#include "common.h"
#include "rawfs.h"

#include "../include/limits.h"
#include "../include/dirent.h"
#include "../include/minix/const.h"
#include "../include/minix/config.h"
#include "../servers/fs/const.h"
#include "../servers/fs/type.h"
#include "../servers/fs/super.h"
#include "../servers/fs/inode.h"
#include "../servers/fs/buf.h"

static unsigned numDZones;				/* # of direct zones */
static unsigned numIndZones;			/* # of indirect zones */
static unsigned inodesPerBlock;
static uint16_t blockSize;	
static SuperBlock super;				/* Superblock of file system */

static Inode currInode;					/* Inode of file under examination */
static char indBuf[MAX_BLOCK_SIZE];		/* Single indirect block. */
static char dblIndBuf[MAX_BLOCK_SIZE];	/* Double indirect block. */
static char dirBuf[MAX_BLOCK_SIZE];		/* Scratch/Directory block. */
#define scratch	dirBuf

static Block_t indAddr, dblIndAddr;		/* Addresses of the indrects. */
static Off_t dirPos;

#define fsBuf(b)	(* (Buf *) (b))

#define zoneShift	(super.s_log_zone_size)	/* Zone to block ratio */

extern void readBlock(Off_t blockNum, char *buf, int bs);

Off_t rawSuper(int *bs) {
/* Initialize variables return the size of a valid Minix file system blocks, 
 * but zero on error.
 */

	/* Read superblock. (The superblock is always at 1 KB offset,
	 * that's why we lie to readBlock and say the block size is 1024
	 * and we want block number 1 (the 'second block', at offset 1 KB).)
	 */
	readBlock(1, scratch, SUPER_BLOCK_BYTES);

	memcpy(&super, scratch, sizeof(super));

	/* Is it really a MINIX file system ? */
	if (super.s_magic == SUPER_V3) {
		*bs = blockSize = super.s_block_size;
		if (blockSize < MIN_BLOCK_SIZE || blockSize > MAX_BLOCK_SIZE)
		  return 0;

		numDZones = NR_DIRECT_ZONES;
		numIndZones = INDIRECT_ZONES(blockSize);
		inodesPerBlock = INODES_PER_BLOCK(blockSize);
		return super.s_zones << zoneShift;
	}
	/* Filesystem not recognized as Minix. */
	return 0;
}

Ino_t rawLookup(Ino_t cwd, char *path) {
/* Translate a pathname to an inode number. This is just a nice utility
 * function, it only needs rawStat and rawReadAddr.
 */
	char name[NAME_MAX + 1], rawName[NAME_MAX + 1];
	char *n;
	struct stat st;
	Ino_t ino;

	ino = path[0] == '/' ? ROOT_INO : cwd;

	while (true) {
		if (ino == 0) {
			errno = ENOENT;
			return 0;
		}

		while (*path == '/') {
			++path;
		}

		if (*path == 0)
		  return ino;
		
		rawStat(ino, &st);

		if (!S_ISDIR(st.st_mode)) {
			errno = ENOTDIR;
			return 0;
		}

		n = name;
		while (*path != 0 && *path != '/') {
			if (n < name + NAME_MAX)
			  *n++ = *path++;
		}
		*n = 0;

		while ((ino = rawReadDir(rawName)) != 0 &&
				strcmp(name, rawName) != 0) {
		}
	}
}

void rawStat(Ino_t iNum, struct stat *st) {
	Block_t blockNum, inoBlockNum;
	Ino_t inoOffset;
	DiskInode *dip;
	int i;

	/* Calculate start of i-list */
	blockNum = START_BLOCK + super.s_imap_blocks + super.s_zmap_blocks;

	/* Calculate block with inode iNum */
	inoBlockNum = (iNum - 1) / inodesPerBlock;
	inoOffset = (iNum - 1) % inodesPerBlock;
	blockNum += inoBlockNum;

	/* Fetch the block */
	readBlock(blockNum, scratch, blockSize);

	if (super.s_magic == SUPER_V3) {
		dip = &fsBuf(scratch).b_inodes[inoOffset];		
		
		currInode.i_mode = dip->mode;
		currInode.i_nlinks = dip->nlinks;
		currInode.i_uid = dip->uid;
		currInode.i_gid = dip->gid;
		currInode.i_size = dip->size;
		currInode.i_atime = dip->atime;
		currInode.i_mtime = dip->mtime;
		currInode.i_ctime = dip->ctime;
	
		for (i = 0; i < NR_TOTAL_ZONES; ++i) {
			currInode.i_zones[i] = dip->zones[i];
		}
	}
	currInode.i_dev = -1;
	currInode.i_num = iNum;

	st->st_dev = currInode.i_dev;
	st->st_ino = currInode.i_num;
	st->st_mode = currInode.i_mode;
	st->st_nlink = currInode.i_nlinks;
	st->st_uid = currInode.i_uid;
	st->st_gid = currInode.i_gid;
	st->st_rdev = (Dev_t) currInode.i_zones[0];
	st->st_size = currInode.i_size;
	st->st_atime = currInode.i_atime;
	st->st_mtime = currInode.i_mtime;
	st->st_ctime = currInode.i_ctime;

	indAddr = dblIndAddr = 0;	
	dirPos = 0;
}

Ino_t rawReadDir(char *name) {
/* Read next directory entry at "dirPos" from file "currInode". */
	Ino_t iNum = 0;
	int blockPos;
	DirEntry *dp;

	if (!S_ISDIR(currInode.i_mode)) {
		errno = ENOTDIR;
		return -1;
	}
	if (!blockSize) {
		errno = 0;
		return -1;
	}
	while (iNum == 0 && dirPos < currInode.i_size) {
		if ((blockPos = dirPos % blockSize) == 0) {
			/* Need to fetch a new directory block. */
			readBlock(rawVir2Abs(dirPos / blockSize), dirBuf, blockSize);
		}
		/* Let dp point to the next entry. */
		dp = (DirEntry *) (dirBuf + blockPos);

		if ((iNum = dp->d_ino) != 0) {
			/* This entry is occupied, return name. */
			strncpy(name, dp->d_name, sizeof(dp->d_name));
			name[sizeof(dp->d_name)] = 0;
		}
		dirPos += DIR_ENTRY_SIZE;
	}
	return iNum;	
}

Off_t rawVir2Abs(Off_t virBlockNum) {
/* Translate a block number in a file to an absolute disk block number.
 * Returns 0 for a hole and -1 if block is past end of file.
 */
	Block_t b;
	Zone_t zone, indZone;
	Block_t blkIdxInZone;
	int i;

	if (!blockSize)
	  return -1;

	/* Check if virBlockNum within file. */
	if (virBlockNum * blockSize >= currInode.i_size)
	  return -1;

	/* Calculate zone in which the datablock number is contained. */
	zone = (Zone_t) (virBlockNum >> zoneShift);

	/* Calculate index of the block number in the zone. */
	blkIdxInZone = virBlockNum - ((Block_t) (zone << zoneShift));

	/* Go get the zone */
	if (zone < numDZones) {	/* Direct block */
		zone = currInode.i_zones[zone];
		b = ((Block_t) (zone << zoneShift)) + blkIdxInZone;
		return b;
	}
	
	/* The zone is not a direct one. */
	zone -= (Zone_t) numDZones;

	/* Is it single indirect? */
	if (zone < (Zone_t) numIndZones) {	/* Single indirect block */
		indZone = currInode.i_zones[INDIR_ZONE_IDX];
	} else {	/* Double indirect block */
		/* Fetch the double indirect block */
		if ((indZone = currInode.i_zones[DBL_IND_ZONE_IDX]) == 0)
		  return 0;
		
		b = (Block_t) indZone << zoneShift;
		if (dblIndAddr != b) {
			readBlock(b, dblIndBuf, blockSize);
			dblIndAddr = b;
		}
		/* Extract the indirect zone number from it */
		zone -= (Zone_t) numIndZones;

		i = zone / (Zone_t) numIndZones;
		indZone = fsBuf(dblIndBuf).b_inds[i];
		zone %= (Zone_t) numIndZones;
	}
	if (indZone == 0)
	  return 0;
	
	/* Extract the datablock number from the indirect zone */
	b = (Block_t) indZone << zoneShift;
	if (indAddr != b) {
		readBlock(b, indBuf, blockSize);
		indAddr = b;
	}
	zone = fsBuf(indBuf).b_inds[zone];

	/* Calculate absolute datablock number */
	b = ((Block_t) zone << zoneShift) + blkIdxInZone;
	return b;
}
	

