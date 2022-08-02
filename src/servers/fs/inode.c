#include "fs.h"

void updateTimes(Inode *ip) {
/* Various system calls are required by the standard to update atime, ctime,
 * or mtime. Since updating a time requires sending a message to the clock
 * task--an expensive business--the times are marked for update by setting
 * bits in i_update. When a stat, fstat, or sync is done, or an inode is
 * released, updateTimes() may be called to actually fill in the times.
 */
	time_t currTime;

	if (ip->i_sp->s_readonly)
	  return;	/* No updates for read-only file system. */

	currTime = clockTime();
	if (ip->i_update & ATIME)
	  ip->i_atime = currTime;
	if (ip->i_update & CTIME)
	  ip->i_ctime = currTime;
	if (ip->i_update & MTIME)
	  ip->i_mtime = currTime;
	ip->i_update = 0;	/* They are all up-to-date now. */
}

static void copyInode(Inode *ip, DiskInode *dip, int rwFlag) {
	int i;

	if (rwFlag == READING) {
		ip->i_mode = dip->d_mode;
		ip->i_uid = dip->d_uid;
		ip->i_nlinks = dip->d_nlinks;
		ip->i_gid = dip->d_gid;
		ip->i_size = dip->d_size;
		ip->i_atime = dip->d_atime;
		ip->i_ctime = dip->d_ctime;
		ip->i_mtime = dip->d_mtime;
		ip->i_dir_zones = NR_DIRECT_ZONES;
		ip->i_ind_zones = INDIRECT_ZONES(ip->i_sp->s_block_size);
		for (i = 0; i < NR_TOTAL_ZONES; ++i) {
			ip->i_zone[i] = dip->d_zone[i];
		}
	} else {
		dip->d_mode = ip->i_mode;
		dip->d_uid = ip->i_uid;
		dip->d_nlinks = ip->i_nlinks;
		dip->d_gid = ip->i_gid;
		dip->d_size = ip->i_size;
		dip->d_atime = ip->i_atime;
		dip->d_ctime = ip->i_ctime;
		dip->d_mtime = ip->i_mtime;
		for (i = 0; i < NR_TOTAL_ZONES; ++i) {
			dip->d_zone[i] = ip->i_zone[i];
		}
	}
} 

void rwInode(register Inode *ip, int rwFlag) {
/* An entry in the inode table is to be copied to or from the disk. */	
	register Buf *bp;
	register SuperBlock *sp;
	DiskInode *dip;
	block_t blockNum, offset;

	/* Get the block where the inode resides. */
	sp = getSuper(ip->i_dev);
	ip->i_sp = sp;		/* Inode must contain super block pointer */
	offset = sp->s_imap_blocks + sp->s_zmap_blocks + IMAP_OFFSET;
	blockNum = (block_t) (ip->i_num - 1) / sp->s_inodes_per_block + offset;
	bp = getBlock(ip->i_dev, blockNum, NORMAL);
	dip = bp->b_inodes + (ip->i_num - 1) % sp->s_inodes_per_block;

	/* Do the read or write. */
	if (rwFlag == WRITING) {
		if (ip->i_update)
		  updateTimes(ip);	/* Times need updating */
		if (!sp->s_readonly)
		  bp->b_dirty = DIRTY;
	}

	/* Copy the inode from the disk block to the in-core table or vice versa. */
	copyInode(ip, dip, rwFlag);

	putBlock(bp, INODE_BLOCK);	// TODO INODE_BLOCK doesn't with WRITE_IMMED
	ip->i_dirty = CLEAN;
}

Inode *getInode(dev_t dev, ino_t num) {
/* Find a slot in the inode table, load the specified inode into it, and
 * return a pointer to the slot. If 'dev' == NO_DEV, just return a free slot.
 */
	register Inode *ip, *xp;

	/* Search the inode table both for (dev, num) and a free slot. */
	xp = NIL_INODE;
	for (ip = &inodes[0]; ip < &inodes[NR_INODES]; ++ip) {
		if (ip->i_count > 0) {	/* Only check used slots for (dev, num) */
			if (ip->i_dev == dev && ip->i_num == num) {
				/* This is the inode that we are looking for. */
				++ip->i_count;
				return ip;	
			}
		} else {
			xp = ip;
		}
	}

	/* Inode we want is not currently in use. Did we find a free slot? */
	if (xp == NIL_INODE) {	/* Inode table completely full */
		errCode = ENFILE;
		return NIL_INODE;
	}

	/* A free inode slot has been located. Load the inode into it. */
	xp->i_dev = dev;
	xp->i_num = num;
	xp->i_count = 1;
	if (dev != NO_DEV)
	  rwInode(xp, READING);	/* Get inode from disk. */
	xp->i_update = 0;	/* All the times are initially up-to-date */

	return xp;
}

void dupInode(Inode *ip) {
/* This routine is a simplified form of getInode() for the case where
 * the inode pointer is already known.
 */
	++ip->i_count;
}

void putInode(register Inode *ip) {
/* The caller is no longer using this inode. If no one else is using it either
 * write it back to the disk immediately. If it has no links, truncate it and
 * return it to the pool of available inodes.
 */
	if (ip == NIL_INODE)	
	  return;
	if (--ip->i_count == 0) {	
		/* i_count == 0 means no one is using it now */
		if (ip->i_nlinks == 0) {
			/* i_nlinks == 0 means free the inode. */
			truncate(ip);	/* Retrun all the disk blocks */
			ip->i_mode = I_NOT_ALLOC;	/* Clear I_TYPE field */
			ip->i_dirty = DIRTY;
			freeInode(ip->i_dev, ip->i_num);
		} else {
			if (ip->i_pipe == I_PIPE)
			  truncate(ip);
		}
		ip->i_pipe = NO_PIPE;	/* Should always be cleared. */
		if (ip->i_dirty == DIRTY)
		  rwInode(ip, WRITING);
	}
}

void freeInode(dev_t dev, ino_t iNum) {
/* Return an inode to the pool of unallocated inodes. */
	register SuperBlock *sp;
	bit_t b;

	/* Locate the appropriate SuperBlock. */
	sp = getSuper(dev);
	if (iNum <= 0 || iNum > sp->s_inodes)
	  return;
	b = iNum;
	freeBit(sp, IMAP, b);
	if (b < sp->s_inode_search)
	  sp->s_inode_search = b;
}

void wipeInode(register Inode *ip) {
/* Erase some fields in the inode.  This function is called from allocInode()
 * when a new inode  is to be allocated, and from truncate(), when an existing
 * inode is to be truncated.
 */
	register int i;

	ip->i_size = 0;
	ip->i_update = ATIME | CTIME | MTIME;	/* Update all times later */
	ip->i_dirty = DIRTY;
	for (i = 0; i < NR_TOTAL_ZONES; ++i) {
		ip->i_zone[i] = NO_ZONE;
	}
}

Inode *allocInode(dev_t dev, mode_t bits) {
/* Allocate a free inode on 'dev', and return a pointer to it. */
	register Inode *ip;
	register SuperBlock *sp;
	bit_t b;
	int major, minor;
	ino_t iNum;
	
	sp = getSuper(dev);
	if (sp->s_readonly) {	/* Can't allocate an inode on a read only device. */
		errCode = EROFS;
		return NIL_INODE;
	}

	/* Acquire an inode from the bit map. */
	b = allocBit(sp, IMAP, sp->s_inode_search);	
	if (b == NO_BIT) {
		errCode = ENFILE;
		major = majorDev(sp->s_dev);
		minor = minorDev(sp->s_dev);
		printf("Out of i-nodes on %sdevice %d/%d\n",
			sp->s_dev == rootDev ? "root " : "", major, minor);
		return NIL_INODE;
	}
	sp->s_inode_search = b;		/* Next time start here */
	iNum = (ino_t) b;

	/* Try to acquire a slot in the inode table. */
	if ((ip = getInode(NO_DEV, iNum)) == NIL_INODE) {
		/* No inode table slots available. Free the inode bit just allocated. */
		freeBit(sp, IMAP, b);
	} else {
		/* An inode slot is available. Put the inode just allocated into it. */
		ip->i_mode = bits;	/* Set up RWX bits */
		ip->i_nlinks = 0;	/* Initial no links */
		ip->i_uid = currFp->fp_euid;	/* File's uid is owner's */
		ip->i_gid = currFp->fp_egid;	/* ditto group id */
		ip->i_dev = dev;	/* Mark which device it is on */
		ip->i_dir_zones = sp->s_dir_zones;	/* # of direct zones */
		ip->i_ind_zones = sp->s_ind_zones;	/* # of indirect zones per blk */
		ip->i_sp = sp;		/* Pointer to super block */

		/* Fields not cleared already are cleared in wipeInode(). */
		wipeInode(ip);
	}

	return ip;
}





