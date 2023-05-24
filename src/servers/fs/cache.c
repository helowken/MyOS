#include "fs.h"
#include <minix/com.h>

/* Note:
 * 1. After init, all bufs are linked together in a LRU chain. FrontBp is
 *    the head of the LRU chain and hashTable[0] = frontBp:
 *		bufHashTable[0] = frontBp;	(initBufPool() in main.c)
 *
 * 2. When getBlock() at first time, no buf is found in hashTable
 *    (assume blockNum > 0), so get from the LRU chain by frontBp:
 *		bp = frontBp;
 *		removeLRU(bp);
 *
 * 3. Since buf.b_block_num == 0, just go:
 *		bufHashTable[hashCode] = bp->b_hash_next;
 *
 * 4. Then fill the buf with dev and blockNum, calculate the hashCode
 *	  and put it to the hashTable:
 *		++bp->b_count;
 *		bufHashTable[hashCode] = bp;
 *
 * 5. When getBlock() again with the same dev and blockNum, the buf can
 *    be found from hashTable, now its b_count == 1, so just increase
 *    its b_count and return it:
 *		++bp->b_count;
 *		return bp;
 *
 * 6. After putBlock(), buf's b_count is decreased by 1, but still > 0, 
 *	  so simply return:
 *		--bp->b_count; 
 *		if (bp->b_count != 0) 
 *		  return;		
 * 
 * 7. Call putBlock() again, buf's b_count is decreased to 0. If it is 
 *	  probably used again shortly, it will be added to the rear of the 
 *	  LRU chain, otherwise to the front. The buf is still in hashTable.
 *
 * 8. If getBlock() with the same dev and blockNum, the buf can be found
 *    from hashTable, now its b_count == 0, meaning that it is in the
 *    LRU chain:
 *		if (bp->b_count == 0) 
 *		  removeLRU(bp); 
 *		++bp->b_count;	
 *		return bp;
 * 
 * 9. Else, no buf can be found from hashTable, needing to get from the 
 *	  LRU chain. If the previous buf was added to the front of the LRU 
 *	  chain, it will be removed from there and filled with different 
 *	  information. Since its dev and blockNum will be changed, it should 
 *	  be remvoed from the hashTable and then put into again with the new 
 *	  dev and blockNum:
 *		while (prev->b_hash_next != NIL_BUF) {
 *			remove from hashTable	
 *		}
 *		go step 4
 */

static void removeLRU(Buf *bp) {
	Buf *next, *prev;

	++bufsInUse;
	next = bp->b_next;
	prev = bp->b_prev;
	if (prev != NIL_BUF)
	  prev->b_next = next;
	else
	  frontBp = next;

	if (next != NIL_BUF)
	  next->b_prev = prev;
	else
	  rearBp = prev;
}

Buf *getBlock(
	dev_t dev,		/* On which device is the block? */
	block_t blockNum,	/* Which block is wanted? */
	int onlySearch	/* If NO_READ, don't read, else act normal */
) {
/* Check to see if the requested block is in the block cache. If so, return
 * a pointer to it. If not, evict some other block and fetch it (unless
 * 'onlySearch' is 1). All the blocks in the cache that are not in use
 * are linked together in a chain, with 'front' pointing to the least recently
 * used block and 'rear' to the most recently used block. If 'onlySearch' is
 * 1, the block being requested will be overwritten in its entirety, so it is
 * only necessary to see if it is in the cache; if it is not, any free buffer
 * will do. It is not necessary to actually read the block in from disk.
 * If 'onlySearch' is PREFETCH, the block need not be read from the disk,
 * and the device is not to be marked on the block, so callers can tell if
 * the block returned is valid.
 * In addition to the LRU chain, there is also a hash chain to link together
 * blocks whose block numbers end with the same bit strings, for fast lookup.
 */
	int hashCode;
	register Buf *bp, *prev;

	/* Search the hash chain for (dev, block). doRead() can use
	 * getBlock(NO_DEV ...) to get an unnamed block to fill with zeros when
	 * someone wants to read from a hole in a file, in which case this search
	 * is skipped.
	 */
	if (dev != NO_DEV) {
		hashCode = (int) blockNum & HASH_MASK;
		bp = bufHashTable[hashCode];
		while (bp != NIL_BUF) {
			if (bp->b_block_num == blockNum && bp->b_dev == dev) {
				/* Block needed has been found. */
				if (bp->b_count == 0)
				  removeLRU(bp);
				++bp->b_count;	/* Record that block is in use */
				return bp;
			} else {
				/* This block is not the one sought. */
				bp = bp->b_hash_next;	/* Move to next block on hash chain */
			}
		}
	}

	/* Desired block is not on available chain. Take oldest block ('front'). */
	if ((bp = frontBp) == NIL_BUF)
	  panic(__FILE__, "all buffers in use", NR_BUFS);
	removeLRU(bp);

	/* Remove the block that was just taken from its hash chain. */
	hashCode = (int) bp->b_block_num & HASH_MASK;
	prev = bufHashTable[hashCode];		
	if (prev == bp) {
		bufHashTable[hashCode] = bp->b_hash_next;
	} else {
		/* The block just taken is not on the front of its hash chain. */
		while (prev->b_hash_next != NIL_BUF) {
			if (prev->b_hash_next == bp) {
				prev->b_hash_next = bp->b_hash_next;	/* Found it */
				break;
			} else {
				prev = prev->b_hash_next;	/* Keep looping */
			}
		}
	}

	/* If the block taken is dirty, make it clean by writing it to the disk.
	 * Avoid hysteresis by flushing all other dirty blocks for the same device.
	 */
	if (bp->b_dev != NO_DEV) {
		if (bp->b_dirty == DIRTY)
		  flushAll(bp->b_dev);
	}

	/* Fill in block's parameters and add it to the hash chain where it goes. */
	bp->b_dev = dev;	/* Fill in device number */
	bp->b_block_num = blockNum;		/* Fill in block number */
	++bp->b_count;		/* Record that block is being used */
	hashCode = (int) blockNum & HASH_MASK;
	bp->b_hash_next = bufHashTable[hashCode];
	bufHashTable[hashCode] = bp;	/* Add to hash list */

	/* Go get the requested block unless searching or prefetching. */
	if (dev != NO_DEV) {
		if (onlySearch == PREFETCH)
		  bp->b_dev = NO_DEV;
		else if (onlySearch == NORMAL)
		  rwBlock(bp, READING);
	}
	return bp;
}

void flushAll(dev_t dev) {
/* Flush all dirty blocks for one device. */
	register Buf *bp;
	static Buf *dirty[NR_BUFS];
	int numDirty;

	for (bp = &bufs[0], numDirty = 0; bp < &bufs[NR_BUFS]; ++bp) {
		if (bp->b_dirty == DIRTY && bp->b_dev == dev)
		  dirty[numDirty++] = bp;
	}
	rwScattered(dev, dirty, numDirty, WRITING);
}

void rwBlock(register Buf *bp, int rwFlag) {
/* Read or write a disk block. This is the only routine in which actual disk
 * I/O is invoked. If an error occurs, a message is printed here, but the error
 * is not reported to the caller. If the error occurred while purging a block
 * from the cache, it is not clear what the caller could do about it anyway.
 */
	int r, op;
	dev_t dev;
	off_t pos;
	int blockSize;

	dev = bp->b_dev;
	blockSize = getBlockSize(dev);

	if (dev != NO_DEV) {
		pos = (off_t) bp->b_block_num * blockSize;
		op = rwFlag == READING ? DEV_READ : DEV_WRITE;
		r = devIO(op, dev, FS_PROC_NR, bp->b_data, pos, blockSize, 0);
		if (r != blockSize) {
			if (r >= 0)
			  r = END_OF_FILE;
			if (r != END_OF_FILE) 
			  printf("Unrecoverable disk error on device %d/%d, block %d\n",
						MAJOR_DEV(dev), MINOR_DEV(dev), bp->b_block_num);
			bp->b_dev = NO_DEV;		/* Invalidate block */
			
			/* Report read errors to interested parties. */
			if (rwFlag == READING)
			  rdwtErr = r;
		}
	}
	bp->b_dirty = CLEAN;
}

void putBlock(
	Buf *bp,	/* Pointer to the buffer to be released */
	int blockType	/* INODE_BLOCK, DIRECTORY_BLOCK, or whatever */
) {
/* Return a block to the list of available blocks. Depending on 'blockType'
 * it may be put on the front or rear of the LRU chain. Blocks that are
 * expected to be needed again shortly (e.g., partially full data blocks)
 * go on the rear; blocks that are unlikely to be needed again shortly
 * (e.g., full data blocks) go on the front. Blocks whose loss can hurt
 * the integrity of the file system (e.g., inode blocks) are written to
 * disk immediately if they are dirty.
 */
	if (bp == NIL_BUF)
	  return;

	--bp->b_count;
	if (bp->b_count != 0)
	  return;		/* Block is still in use */

	--bufsInUse;	

	/* Put this block back on the LRU chain. If the ONE_SHOT bit is set in
	 * 'blockType', the block is not likely to be needed again shortly, so 
	 * put it on the front of the LRU chain where it will be the first one 
	 * to be taken when a free buffer is needed later.
	 */
	if (bp->b_dev == DEV_RAM || blockType & ONE_SHOT) {
		/* Block probably won't be needed quickly. Put it on front of chain.
		 * It will be the next block to be evicted from the cache.
		 */
		bp->b_prev = NIL_BUF;
		bp->b_next = frontBp;
		if (frontBp == NIL_BUF)
		  rearBp = bp;	/* LRU chain was empty */
		else
		  frontBp->b_prev = bp;
		frontBp = bp;
	} else {
		/* Block probably will be needed quickly. Put it on rear of chain.
		 * It will not be evicted from the cache from a long time.
		 */
		bp->b_prev = rearBp;
		bp->b_next = NIL_BUF;
		if (rearBp == NIL_BUF)
		  frontBp = bp;
		else
		  rearBp->b_next = bp;
		rearBp = bp;
	}

	/* Some blocks are so important (e.g., inodes, indirect blocks) that they
	 * should be written to the disk immediately to avoid messing up the file
	 * system in the event of a crash.
	 */
	if ((blockType & WRITE_IMMED) && 
				bp->b_dirty == DIRTY && 
				bp->b_dev != NO_DEV) {
		rwBlock(bp, WRITING);
	}
}

void rwScattered(dev_t dev, Buf **bufQueue, int queueSize, int rwFlag) {
/* Read or write scattered data from a device. */
	register Buf *bp;
	int gap;
	register int i;
	register IOVec *iop;
	static IOVec ioVec[NR_IO_REQS];
	int j, r;
	int blockSize;

	blockSize = getBlockSize(dev);

	/* (Shell) sort buffers on b_block_num. */
	gap = 1;
	do {
		gap = 3 * gap + 1;
	} while (gap <= queueSize);

	while (gap != 1) {
		gap /= 3;
		for (j = gap; j < queueSize; ++j) {
			for (i = j - gap;
				 i >= 0 && bufQueue[i]->b_block_num > bufQueue[i + gap]->b_block_num;
				 i -= gap) {
				bp = bufQueue[i];
				bufQueue[i] = bufQueue[i + gap];
				bufQueue[i + gap] = bp;
			}
		}
	}

	/* Set up I/O vector and do I/O. The result of devIO is OK if everything
	 * went fine, otherwise the error code for the first failed transfer.
	 */
	while (queueSize > 0) {
		for (j = 0, iop = ioVec; j < NR_IO_REQS && j < queueSize; ++j, ++iop) {
			bp = bufQueue[j];
			if (bp->b_block_num != bufQueue[0]->b_block_num + j)
			  break;
			iop->iov_addr = (vir_bytes) bp->b_data;
			iop->iov_size = blockSize;
		}
		r = devIO(rwFlag == WRITING ? DEV_SCATTER : DEV_GATHER,
				dev, FS_PROC_NR, ioVec,
				(off_t) bufQueue[0]->b_block_num * blockSize, j, 0);
		/* Harvest the results. devIO reports the first error it may have
		 * encountered, but we only care if it's the first block that failed.
		 */
		for (i = 0, iop = ioVec; i < j; ++i, ++iop) {
			bp = bufQueue[i];
			if (iop->iov_size != 0) {
				/* Transfer failed. An error? Do we care? */
				if (r != OK && i == 0) {	
					printf("FS: I/O error on device %d/%d, block %d\n",
						MAJOR_DEV(dev), MINOR_DEV(dev), bp->b_block_num);
					bp->b_dev = NO_DEV;		/* Invalidate block */
				}
				break;
			}
			if (rwFlag == READING) {
				bp->b_dev = dev;	/* Validate block */
				putBlock(bp, PARTIAL_DATA_BLOCK);
			} else {
				bp->b_dirty = CLEAN;
			}
		}
		bufQueue += i;
		queueSize -= i;
		if (rwFlag == READING) {
			/* Don't bother reading more than the device is willing to 
			 * give at this time. Don't forget to release those extras.
			 */
			while (queueSize > 0) {
				putBlock(*bufQueue++, PARTIAL_DATA_BLOCK);
				--queueSize;
			}
		}
		if (rwFlag == WRITING && i == 0) {
			/* We're not making progress, this means we might keep
			 * looping. Buffers remain dirty if un-written. Buffers are
			 * lost if invalidate()d or LRU-removed while dirty. This
			 * is better than keeping unwritable blocks around forever.
			 */
			break;
		}
	}
}

void invalidate(
	dev_t dev	/* Device whose blocks are to be purged */
) {
/* Remove all the blocks belonging to some device from the cache. */
	register Buf *bp;

	for (bp = &bufs[0]; bp < &bufs[NR_BUFS]; ++bp) {
		if (bp->b_dev == dev)
		  bp->b_dev = NO_DEV;
	}
}

void freeZone(dev_t dev, zone_t num) {
/* Return a zone. */
	register SuperBlock *sp;
	bit_t bit;

	/* Locate the appropriate SuperBlock and return bit. */
	sp = getSuper(dev);
	if (num < sp->s_first_data_zone || num >= sp->s_zones)
	  return;
	bit = (bit_t) (num - (sp->s_first_data_zone - 1));
	freeBit(sp, ZMAP, bit);
	if (bit < sp->s_zone_search)
	  sp->s_zone_search = bit;
}

zone_t allocZone(
	dev_t dev,
	zone_t z	/* Try to allocate new zone near this one */
) {
/* Allocate a new zone on the indicated device and return its number. */

	SuperBlock *sp;
	bit_t bit, b;

	/* Note that the routine allocBit() returns 1 for the lowest possible
	 * zone, which corresponds to sp->s_first_data_zone. To convert a value
	 * between the bit number, 'b', used by allocBit() and the zone number, 'z',
	 * stored in the inode, use the formula:
	 *		z = b + sp->s_first_data_zone - 1
	 * AllocBit() never returns 0, since this is used for NO_BIT (failure).
	 */
	sp = getSuper(dev);

	/* If z is sp->s_first_data_zone, skip initial part of the map known to be 
	 * fully in used. 
	 */
	if (z == sp->s_first_data_zone) 
	  bit = sp->s_zone_search;
	else
	  bit = (bit_t) z - (sp->s_first_data_zone - 1);
	
	b = allocBit(sp, ZMAP, bit);
	if (b == NO_BIT) {
		errCode = ENOSPC;
		printf("No space on %sdevice %d/%d\n", 
			sp->s_dev == rootDev ? "root " : "", MAJOR_DEV(dev), MINOR_DEV(dev));
		return NO_ZONE;
	}
	if (z == sp->s_first_data_zone)
	  sp->s_zone_search = b;	/* For next time */

	/* 'sp->s_first_data_zone - 1' since zone 0 occupy no space. */
	return sp->s_first_data_zone - 1 + ((zone_t) b);	
}








