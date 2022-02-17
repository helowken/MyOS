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

static unsigned numDZones;
static unsigned numIndZones;
static unsigned inodesPerBlock;
static uint16_t blockSize;	
static SuperBlock super;				/* Superblock of file system */

static Inode currInode;					/* Inode of file under examination */
static char dirBuf[MAX_BLOCK_SIZE];		/* Scratch/Directory block. */
#define scratch	dirBuf

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
	char name[NAME_MAX + 1];
	//char rawName[NAME_MAX + 1];
	char *n;
	struct stat st;
	Ino_t ino;

	ino = path[0] == '/' ? ROOT_INO : cwd;

	while (1) {
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
		//TODO
	}
}

void rawStat(Ino_t iNum, struct stat *st) {
	Block_t blockNum;
	Block_t inoBlockNum;
	Ino_t inoOffset;
	DiskInode *ip;

	/* Calculate start of i-list */
	blockNum = START_BLOCK + super.s_imap_blocks + super.s_zmap_blocks;

	/* Calculate block with inode iNum */
	inoBlockNum = (iNum - 1) / inodesPerBlock;
	inoOffset = (iNum - 1) % inodesPerBlock;
	blockNum += inoBlockNum;

	/* Fetch the block */
	readBlock(blockNum, scratch, blockSize);

	if (super.s_magic == SUPER_V3) {
		ip = &fsBuf(scratch).b_inodes[inoOffset];		
		
		currInode.i_mode = ip->mode;

	}
}





