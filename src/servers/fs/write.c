#include "fs.h"
#include "string.h"

int doWrite() {
/* Perform the write(fd, buffer, count) system call. */
	return readWrite(WRITING);
}

static int writeMap(register Inode *ip, off_t position, zone_t newZone) {
/* Write a new zone into an inode. */
	Buf *bp;
	int scale, zones, indZones, zIdx, idxOfDbl;
	long excess, zone;
	bool single, newInd, newDbl;
	zone_t z, z1;
	block_t b;

	ip->i_dirty = DIRTY;	/* Inode will be changed */
	bp = NIL_BUF;
	scale = ip->i_sp->s_log_zone_size;		/* For zone-block conversion */
	/* Relative zone # to insert */
	zone = (position / ip->i_sp->s_block_size) >> scale;
	zones = ip->i_dir_zones;	/* # direct zones in the inode */
	indZones = ip->i_ind_zones;		/* # indirect zones per indirect block */

	/* Is 'position' to be found in the inode itself? */
	if (zone < zones) {
		zIdx = (int) zone;	
		ip->i_zone[zIdx] = newZone;
		return OK;
	}

	/* It is not in the inode, so it must be single or double indirect. */
	excess = zone - zones;
	newInd = false;
	newDbl = false;

	if (excess < indZones) {
		/* 'position' can be located via the single indirect block. */
		z1 = ip->i_zone[zones];
		single = true;
	} else {
		/* 'position' can be located via the double indirect block. */
		if ((z = ip->i_zone[zones + 1]) == NO_ZONE) {
			/* Create the double indirect block. */
			if ((z = allocZone(ip->i_dev, ip->i_zone[0])) == NO_ZONE)
			  return errCode;
			ip->i_zone[zones + 1] = z;
			newDbl = true;
		}

		/* Either way, 'z' is zone number for double indirect block. */
		excess -= indZones;
		idxOfDbl = (int) (excess / indZones);
		excess %= indZones;	/* Now excess is the index of indirect zones */
		if (idxOfDbl >= indZones)
		  return EFBIG;
		b = (block_t) z << scale;
		bp = getBlock(ip->i_dev, b, (newDbl ? NO_READ : NORMAL));
		if (newDbl)
		  zeroBlock(bp);
		z1 = readIndirZone(bp, idxOfDbl);	
		single = false;
	}

	/* z1 is now single indirect zone; 'excess' is index. */
	if (z1 == NO_ZONE) {
		/* Create ind blk and store zone # in inode or dbl indir blk. */
		z1 = allocZone(ip->i_dev, ip->i_zone[0]);
		if (single) 
		  ip->i_zone[zones] = z1;	/* Update inode */
		else {
			bp->b_inds[idxOfDbl] = z1;	/* Update dbl indir */
			bp->b_dirty = DIRTY;
		}
		newInd = true;
		if (z1 == NO_ZONE) {
			putBlock(bp, INDIRECT_BLOCK);	/* Release dbl indirect blk */
			return errCode;		/* Couldn't create single ind */
		}
	}
	putBlock(bp, INDIRECT_BLOCK);	/* Release double indirect blk */

	/* z1 is indirect block's zone number. */
	b = (block_t) z1 << scale;
	bp = getBlock(ip->i_dev, b, (newInd ? NO_READ : NORMAL));
	if (newInd)
	  zeroBlock(bp);
	bp->b_inds[excess] = newZone;
	bp->b_dirty = DIRTY;
	putBlock(bp, INDIRECT_BLOCK);

	return OK;
}

Buf *newBlock(register Inode *ip, off_t pos) {
/* Acquire a new block and return a pointer to it. Doing so may require
 * allocating a complete zone, and then returning the initial block.
 * On the other hand, the current zone may still have some unused blocks.
 */
	register Buf *bp;
	int scale, r;
	block_t b, baseBlock;
	zone_t z, zoneSize;

	/* Is another block available in the current zone? */
	if ((b = readMap(ip, pos)) == NO_BLOCK) {
		/* Choose first zone if possible. 
		 * Lose if the file is non-empty but the first zone number is NO_ZONE
		 * corresponding to a zone full of zeros. It would be better to 
		 * search near the last real zone.
		 */
		if (ip->i_zone[0] == NO_ZONE) 
		  z = ip->i_sp->s_first_data_zone; 
		else 
		  z = ip->i_zone[0];	/* Hunt near first zone */

		if ((z = allocZone(ip->i_dev, z)) == NO_ZONE)
		  return NIL_BUF;
		if ((r = writeMap(ip, pos, z)) != OK) {
			freeZone(ip->i_dev, z);
			errCode = r;
			return NIL_BUF;
		}

		/* If we are not writing at EOF, clear the zone, just to be safe. */
		if (pos != ip->i_size)
		  clearZone(ip, pos, 1);
		scale = ip->i_sp->s_log_zone_size;
		baseBlock = (block_t) z << scale;
		zoneSize = (zone_t) ip->i_sp->s_block_size << scale;
		b = baseBlock + (block_t)((pos % zoneSize) / ip->i_sp->s_block_size);
	}
	bp = getBlock(ip->i_dev, b, NO_READ);
	zeroBlock(bp);
	return bp;
}

void zeroBlock(Buf *bp) {
/* Zero a block. */
	memset(bp->b_data, 0, MAX_BLOCK_SIZE);
	bp->b_dirty = DIRTY;
}

void clearZone(
	Inode *ip,
	off_t pos,
	int flag	/* 0 if called by readWrite, 1 by newBlock */
) {
/* Zero a zone, possibly starting in the middle. The parameter 'pos' gives
 * a byte in the first block to be zeroed. ClearZone() is called from
 * readWrite and newBlock().
 */
	register Buf *bp;
	register block_t b, bLow, bHigh;
	register int scale;
	register off_t next;
	register zone_t zoneSize;

	/* If the block size and zone size are the same, clearZone() not needed. */
	scale = ip->i_sp->s_log_zone_size;
	if (scale == 0)
	  return;

	zoneSize = (zone_t) ip->i_sp->s_block_size << scale;
	if (flag == 1)
	  pos = (pos / zoneSize) * zoneSize;
	next = pos + ip->i_sp->s_block_size - 1;

	/* 'pos' is in the last block of a zone, do not clear the zone. */
	if (next / zoneSize != pos / zoneSize)
	  return;
	if ((bLow = readMap(ip, next)) == NO_BLOCK)
	  return;
	bHigh = (((bLow >> scale) + 1) << scale) - 1;

	/* Clear all the blocks between 'bLow' and 'bHigh'. */
	for (b = bLow; b <= bHigh; ++b) {
		bp = getBlock(ip->i_dev, b, NO_READ);
		zeroBlock(bp);
		putBlock(bp, FULL_DATA_BLOCK);
	}
}




