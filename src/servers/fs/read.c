#include "fs.h"
#include "fcntl.h"
#include "minix/com.h"
#include "param.h"

int doRead() {
	return readWrite(READING);
}

static int rwChunk(
	register Inode *ip,
	off_t position,		/* Position within file to read or write */
	unsigned off,		/* Off within the current block */
	int chunk,			/* # of bytes to read or write */
	unsigned bytesLeft,	/* Max number of bytes wanted after position */
	int rwFlag,			/* READING or WRITING */
	char *buf,			/* Virtual address of the user buffer */
	int seg,			/* T or D segment in user space */
	int usr,			/* Which user process */
	int blockSize,		/* Block size of FS operating on */
	int *completed		/* # of bytes copied */
) {
/* Read or write (part of) a block. */

	register Buf *bp;
	int r = OK;
	int n, blockSpec;
	block_t b;
	dev_t dev;

	*completed = 0;

	blockSpec = (ip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
	if (blockSpec) {
		b = position / blockSize;
		dev = (dev_t) ip->i_zone[0];
	} else {
		b = readMap(ip, position);
		dev = ip->i_dev;
	}

	if (! blockSpec && b == NO_BLOCK) {
		if (rwFlag == READING) {
			/* Reading from a nonexistent block. Must read as all zeros. */
			bp = getBlock(NO_DEV, NO_BLOCK, NORMAL);	/* Get a buffer */
			zeroBlock(bp);
		} else {
			/* Writing to a nonexistent block. Create and enter in inode. */
			if ((bp = newBlock(ip, position)) == NIL_BUF)
			  return errCode;
		}
	} else if (rwFlag == READING) {
		/* Read and read ahead if convenient. */
		bp = doReadAhead(ip, b, position, bytesLeft);
	} else {
		/* Normally an existing block to be partially overwritten is first read
		 * in. However, a full block need not be read in. If it is already in
		 * the cache, acquire it, otherwise just acquire a free buffer.
		 */
		n = (chunk == blockSize ? NO_READ : NORMAL);
		if (! blockSpec && off == 0 && position >= ip->i_size)
		  n = NO_READ;
		bp = getBlock(dev, b, n);
	}

	/* In all cases, bp now points to a valid buffer. */
	if (bp == NIL_BUF)
	  panic(__FILE__, "bp not valid in rwChunk, this can't happen", NO_NUM);

	if (rwFlag == WRITING && 
				chunk != blockSize && 
				! blockSpec &&
				position >= ip->i_size &&
				off == 0) {
		zeroBlock(bp);
	}

	if (rwFlag == READING) {
		/* Copy a chunk from the block buffer to user space. */
		r = sysVirCopy(FS_PROC_NR, D, (phys_bytes) (bp->b_data + off), 
					usr, seg, (phys_bytes) buf, (phys_bytes) chunk);
	} else {
		/* Copy a chunk from user space to the block buffer. */
		r = sysVirCopy(usr, seg, (phys_bytes) buf, 
					FS_PROC_NR, D, (phys_bytes) (bp->b_data + off),
					(phys_bytes) chunk);
		bp->b_dirty = DIRTY;
	}

	n = (off + chunk == blockSize ? FULL_DATA_BLOCK : PARTIAL_DATA_BLOCK);
	putBlock(bp, n);

	return r;
}

int readWrite(int rwFlag) {
/* Perform read(fd, buf, count) or write(fd, buf, count) call. */

	Inode *ip;
	Filp *fp;
	off_t position, fileSize, bytesLeft;
	unsigned int off, cumIO;
	mode_t mode;
	int regular;
	int blockSize;
	int r, usr, seg, chunk, op, oFlags, blockSpec = 0, charSpec = 0;
	int completed, r2 = OK;
	phys_bytes p;

	if (bufsInUse < 0) 
	  panic(__FILE__, "start - bufsInUse negative", bufsInUse);

	/* MM loads segments by putting funny things in upper 10 bits of "fd". */
	if (who == PM_PROC_NR && (inMsg.m_fd & (~BYTE))) {
		usr = inMsg.m_fd >> 7;
		seg = (inMsg.m_fd >> 5) & 03;
		inMsg.m_fd &= 037;	/* Get rid of user and segment bits */
	} else {
		usr = who;	/* Normal case */
		seg = D;
	}

	/* If the file descriptor is valid, get the inode, size and mode. */
	if (inMsg.m_nbytes < 0)
	  return EINVAL;
	if ((fp = getFilp(inMsg.m_fd)) == NIL_FILP)
	  return errCode;
	if ((fp->filp_mode && (rwFlag == READING ? R_BIT : W_BIT)) == 0)
	  return fp->filp_mode == FILP_CLOSED ? EIO : EBADF;

	if (inMsg.m_nbytes == 0)
	  return 0;		/* So char special files need not check for 0 */

	/* Check if user process has the memory it needs.
	 * If not, copying will fail later.
	 * Do this after 0-check above because umap doesn't want to map 0 bytes.
	 */
	if ((r = sysUMap(usr, seg, (vir_bytes) inMsg.m_buffer, 
						inMsg.m_nbytes, &p)) != OK)
	  return r;

	position = fp->filp_pos;
	oFlags = fp->filp_flags;
	ip = fp->filp_inode;
	fileSize = ip->i_size;
	r = OK;
	if (ip->i_pipe == I_PIPE) {
		//TODO;
	} else {
		cumIO = 0;
	}
	op = (rwFlag == READING ? DEV_READ : DEV_WRITE);
	mode = ip->i_mode & I_TYPE;
	regular = mode == I_REGULAR || mode == I_NAMED_PIPE;

	if ((charSpec = (mode == I_CHAR_SPECIAL))) {
		if (ip->i_zone[0] == NO_DEV)
		  panic(__FILE__, "readWrite tries to read from "
					  "character device NO_DEV", NO_NUM);
		blockSize = getBlockSize(ip->i_zone[0]);
	} else if ((blockSpec = (mode == I_BLOCK_SPECIAL))) {
		fileSize = ULONG_MAX;
		if (ip->i_zone[0] == NO_DEV)
		  panic(__FILE__, "readWrite tries to read from "
					  "block device NO_DEV", NO_NUM);
		blockSize = getBlockSize(ip->i_zone[0]);
	} else { 
		blockSize = ip->i_sp->s_block_size;
	}

	rdwtErr = OK;	/* Set to EIO if disk error occurs */

	/* Check for character special files. */
	if (charSpec) {
		dev_t dev;
		dev = (dev_t) ip->i_zone[0];
		r = devIO(op, dev, usr, inMsg.m_buffer, position, 
					inMsg.m_nbytes, oFlags);
		if (r >= 0) {
			cumIO = r;
			position += r;
			r = OK;
		}
	} else {
		if (rwFlag == WRITING && ! blockSpec) {
			/* Check in advance to see if file will grow too big. */
			if (position > ip->i_sp->s_max_size - inMsg.m_nbytes)
			  return EFBIG;

			/* Check for O_APPEND flag. */
			if (oFlags & O_APPEND)
			  position = fileSize;

			/* Clear the zone containing present EOF if hole about
			 * to be created. This is necessary because all unwritten
			 * blocks prior to the EOF must read as zeros.
			 */
			if (position > fileSize)
			  clearZone(ip, fileSize, 0);
		}

		/* Pipes are a little different. Check. */
		if (ip->i_pipe == I_PIPE) {
			//TODO
		}

		// TODO if partial

		/* Split the transfer into chunks that don't span two blocks. */
		while (inMsg.m_nbytes) {
			off = (unsigned int) (position % blockSize);	/* offset in block */
			// if partial pipe
			// else
			chunk = MIN(inMsg.m_nbytes, blockSize - off);
			if (chunk < 0)
			  chunk = blockSize - off;

			if (rwFlag == READING) {
				if (position >= fileSize)
				  break;	/* We are beyond EOF */
				bytesLeft = fileSize - position;
				if (chunk >= bytesLeft)
				  chunk = (int) bytesLeft;
			}

			/* Read or write 'chunk' bytes. */
			r2 = rwChunk(ip, position, off, chunk, (unsigned) inMsg.m_nbytes,
					rwFlag, inMsg.m_buffer, seg, usr, blockSize, &completed);
			if (r2 != OK)
			  break;	/* EOF reached */

			/* Update counters and pointers. */
			inMsg.m_buffer += chunk;	/* User buffer address */
			inMsg.m_nbytes -= chunk;	/* Bytes yet to be r/w */
			cumIO += chunk;		/* Bytes r/w so far */
			position += chunk;	/* Position within the file */

			//TODO if partial pipe
		}
	}

	/* On write, update file size and access time. */
	if (rwFlag == WRITING) {
		if (regular || mode == I_DIRECTORY) {
			if (position > fileSize)
			  ip->i_size = position;
		}
	} else {
		// TODO if I_PIPE
	}
	fp->filp_pos = position;

	/* Check to see if read ahead is called for, and if so, set it up. */
	if (rwFlag == READING && 
				ip->i_seek == NO_SEEK && 
				position % blockSize == 0 &&
				(regular || mode == I_DIRECTORY)) {
		readAheadInode = ip;
		readAheadPos = position;
	}
	ip->i_seek = NO_SEEK;

	if (rdwtErr != OK)
	  r = rdwtErr;	/* Check for disk error */
	if (rdwtErr == END_OF_FILE)
	  r = OK;
	/* If user-space copying failed, r/w failed. */
	if (r == OK && r2 != OK)
	  r = r2;

	if (r == OK) {
		if (rwFlag == READING)
		  ip->i_update |= ATIME;
		else if (rwFlag == WRITING)
		  ip->i_update |= CTIME | MTIME;
		ip->i_dirty = DIRTY;	/* Inode is thus now dirty */
		// TODO if partial pipe
		currFp->fp_cum_io_partial = 0;
		return cumIO;
	}
	if (bufsInUse < 0)
	  panic(__FILE__, "end - bufsInUse negative", bufsInUse);

	return r;
}

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
		zoneNum = readIndirZone(bp, idxInDblInd);	/* Zone for single indirect */
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
	idxInInd = (int) excess;
	zoneNum = readIndirZone(bp, idxInInd);	/* Get final zone pointed to */
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
	blocksAhead = 1;//TODO

	/* Acquire block buffers. */
	while (true) {
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

zone_t readIndirZone(Buf *bp, int index) {
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


