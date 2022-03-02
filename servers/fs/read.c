#include "fs.h"
#include "fcntl.h"
#include "minix/com.h"
#include "file.h"
#include "param.h"

void readAhead() {
/* Read a block into the cache before it is needed. */
	int blockSize;
	register Inode *ip;
	Buf *bp;
	block_t blockNum;

	ip = readAheadInode;	/* Pointer to inode to read ahead from */
	blockSize = getBlockSize(ip->i_dev);
	readAheadInode = NIL_INODE;	/* Turn off read ahead */
	if ((blockNum = readMap(ip, readAheadPos)) == NO_BLOCK)
	  return;	/* At EOF */
	bp = doReadAhead(ip, blockNum, readAheadPos, blockSize);
	putBlock(bp, PARTIAL_DATA_BLOCK);
}

block_t readMap(Inode *ip, off_t pos) {
/* Given an inode and a position within the corresponding file, locate the
 * block (not zone) number in which that position is to be found and return it.
 */
	register Buf *bp;
	register zone_t zoneNum;
	int scale, blockOffInZone, dirZones, indZones, idxInInd, idxInDblInd;
	block_t blockNum;
	long blockForPos, zoneForPos, excess;

	scale = ip->i_sp->s_log_zone_size;	
	blockForPos = pos / ip->i_sp->s_block_size;	/* Relative block # in file */
	zoneForPos = blockForPos >> scale;	/* zone for 'pos' */
	blockOffInZone = (int) (blockForPos - (zoneForPos << scale));	/* Relative block # within zone */
	dirZones = ip->i_dir_zones;
	indZones = ip->i_ind_zones;

	/* Is 'pos' to be found in the inode itself? */
	if (zoneForPos < dirZones) {
		zoneNum = ip->i_zone[(int) zoneForPos];
		if (zoneNum == NO_ZONE)
		  return NO_BLOCK;
		blockNum = ((block_t) zoneNum << scale) + blockOffInZone;
		return blockNum;
	}

	/* It is not in the inode, so it must be single or double indirect. */
	excess = zoneForPos - dirZones;		/* Direct zones doesn't count */
	
	if (excess < indZones) {
		/* 'pos' can be located via the single indirect block. */
		zoneNum = ip->i_zone[INDIR_ZONE_IDX];
	} else {
		/* 'pos' can be located via the double indirect block. */
		if ((zoneNum = ip->i_zone[DBL_IND_ZONE_IDX]) == NO_ZONE)
		  return NO_BLOCK;
		excess -= indZones;		/* Single indirect doesn't count */
		blockNum = (block_t) zoneNum << scale;
		bp = getBlock(ip->i_dev, blockNum, NORMAL);	/* Get double indirect block */
		idxInDblInd = (int) (excess / indZones);
		zoneNum = readIndir(bp, idxInDblInd);	/* Zone for single indirect */
		putBlock(bp, INDIRECT_BLOCK);	/* Release double indirect block */
		idxInInd = excess % indZones;	/* Index into single indirect block */
	}

	/* 'zoneNum' is zone num for single indirect block;
	 * 'idxInInd' is index into it.
	 */
	if (zoneNum == NO_ZONE)
	  return NO_BLOCK;
	blockNum = (block_t) zoneNum << scale;	/* Block # for single indirect block */
	bp = getBlock(ip->i_dev, blockNum, NORMAL);	/* Get single indirect block */
	zoneNum = readIndir(bp, idxInInd);	/* Get final zone pointed to */
	putBlock(bp, INDIRECT_BLOCK);		/* Release single indirect block */
	if (zoneNum == NO_ZONE)
	  return NO_BLOCK;
	blockNum = ((block_t) zoneNum << scale) + blockOffInZone;
	return blockNum;
}

Buf *doReadAhead(
	Inode *ip,		/* Pointer to inode for file to be read */
	block_t baseBlock,	/* Block at current position */
	off_t position,		/* Position within file */
	unsigned bytesAhead		/* Bytes beyond position for immediate use */
) {
/* Fetch a block from the cache or the device. If a physical read is 
 * required, prefetch as many more blocks as convenient into the cache.
 * This usually covers bytesAhead and is at least BLOCKS_MINIMUM.
 * The device driver may decide it knows better and stop reading at a
 * cylinder boundary (or after an error). rwScattered() puts an optional
 * flag on all reads to allow this.
 */
	int blockSize;
/* Minimum number of blocks to prefetch. */
#define BLOCKS_MINIMUM	(NR_BUFS < 50 ? 18 : 32)
	int blockSpecial, scale, queueSize;
	unsigned int blocksAhead, fragment;
	off_t indPos;
	dev_t dev;
	block_t blockNum, blocksLeft;
	Buf *bp;
	static Buf *readQueue[NR_BUFS];
	
	blockSpecial = (ip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
	dev = blockSpecial ? (dev_t) ip->i_zone[0] : ip->i_dev;
	blockSize = getBlockSize(dev);
	
	blockNum = baseBlock;
	bp = getBlock(dev, blockNum, PREFETCH);
	if (bp->b_dev != NO_DEV)
	  return bp;

	/* The best guess for the number of blocks to prefetch: A lot.
	 * It is impossible to tell what the device looks like, so we don't even
	 * try to guess the geometry, but leave it to the driver.
	 *
	 * The floppy driver can read a full track with no rotational delay, and it
	 * avoids reading partial tracks if it can, so handing it enough buffers to
	 * read two tracks is perfect. (Two, becuase some diskette types have an
	 * odd number of sectors per track, so a block may span tracks.)
	 *
	 * The disk drivers don't try to be smart. With todays disks it is
	 * impossible to tell what the real geometry looks like, so it is best to
	 * read as much as you can. With luck the caching on the drive allows for
	 * a little time to start the next read.
	 *
	 * The current solution below is a bit of a hack, it just reads blocks from
	 * the current file position hoping that more of the file can be found. A
	 * better solution must look at the already available zone pointers and
	 * indirect blocks (but don't call readMap!).
	 */
	fragment = position % blockSize;
	position -= fragment;
	bytesAhead += fragment;

	blocksAhead = (bytesAhead + blockSize - 1) / blockSize;

	if (blockSpecial && ip->i_size == 0) {
		blocksLeft = NR_IO_REQS;
	} else {
		blocksLeft = (ip->i_size - position + blockSize - 1) / blockSize;

		/* Go for the first indirect block if we are in its neighborhood. */
		if (!blockSpecial) {
			scale = ip->i_sp->s_log_zone_size;
			indPos = (off_t) (ip->i_dir_zones << scale) * blockSize;
			if (position <= indPos && ip->i_size > indPos) {
				++blocksAhead;
				++blocksLeft;
			}
		}
	}

	/* No more than the maximum request. */
	if (blocksAhead > NR_IO_REQS)
	  blocksAhead = NR_IO_REQS;
	
	/* Read at least the minimum number of blocks, but not after a seek. */
	if (blocksAhead < BLOCKS_MINIMUM && ip->i_seek == NO_SEEK)
	  blocksAhead = BLOCKS_MINIMUM;

	/* Can't go past end of file. */
	if (blocksAhead > blocksLeft)
	  blocksAhead = blocksLeft;

	queueSize = 0;

	/* Acquire block buffers. */
	while (1) {
		readQueue[queueSize++] = bp;

		if (--blocksAhead == 0)
		  break;

		/* Don't trash the cache, leave 4 free. */
		if (bufsInUse >= NR_BUFS - 4)
		  break;

		++blockNum;
		bp = getBlock(dev, blockNum, PREFETCH);
		if (bp->b_dev != NO_DEV) {
			/* Oops, block already in the cache, get out. */
			putBlock(bp, FULL_DATA_BLOCK);
			break;
		}
	}
	rwScattered(dev, readQueue, queueSize, READING);
	return getBlock(dev, baseBlock, NORMAL);
}

zone_t readIndir(Buf *bp, int index) {
/* Given a pointer to an indirect block, read one entry. */
	SuperBlock *sp;
	zone_t zone;

	sp = getSuper(bp->b_dev);
	zone = bp->b_inds[index];

	if (zone != NO_ZONE &&
		(zone < sp->s_first_data_zone || zone >= sp->s_zones)) {
		printf("Illegal zone number %ld in indirect block, index %d\n",
					(long) zone, index);
		panic(__FILE__, "check file system", NO_NUM);
	}
	return zone;
}


