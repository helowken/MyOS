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

Inode *getInode(dev_t dev, int num) {
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


