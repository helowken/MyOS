#include "fs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include "param.h"

static dev_t nameToDev(char *path) {
/* Convert the block special file 'path' to a device number. If 'path'
 * is not a block special file, return error code in 'errCode'.
 */
	register Inode *rip;
	register dev_t dev;

	/* If 'path' can't be opened, give up immediately. */
	if ((rip = eatPath(path)) == NIL_INODE)
	  return NO_DEV;

	/* If 'path' is not a block special file, return error. */
	if ((rip->i_mode & I_TYPE) != I_BLOCK_SPECIAL) {
		errCode = ENOTBLK;
		putInode(rip);
		return NO_DEV;
	}

	/* Extract the device number. */
	dev = (dev_t) rip->i_zone[0];
	putInode(rip);
	return dev;
}

int doMount() {
/* Perform the mount(special, file, rwFlag) system call. */
	register Inode *rip, *rootIp;
	SuperBlock *xp, *sp;
	dev_t dev;
	mode_t bits;
	int rdir, mdir;		/* TRUE iff {root|mount} file is dir */
	int r;

	/* Only the super-user may do MOUNT. */
	if (! superUser)
	  return EPERM;

	/* If 'special' is not for a block special file, return error. */
	if (fetchName(inMsg.m_name1, inMsg.m_name1_length, M1) != OK)
	  return errCode;
	if ((dev = nameToDev(userPath)) == NO_DEV)
	  return errCode;

	/* Scan super block table to see if dev already mounted & find a free slot. */
	sp = NIL_SUPER;
	for (xp = &superBlocks[0]; xp < &superBlocks[NR_SUPERS]; ++xp) {
		if (xp->s_dev == dev)
		  return EBUSY;		/* Already mounted */
		if (xp->s_dev == NO_DEV)
		  sp = xp;			/* Record free slot */
	}
	if (sp == NIL_SUPER)
	  return ENFILE;	/* No super block available. */

	/* Open the device the file system lives on. */
	if (devOpen(dev, who, inMsg.m_rd_only ? R_BIT : (R_BIT | W_BIT)) != OK)
	  return EINVAL;

	/* Make the cache forget about blocks it has open on the filesystem */
	doSync();
	invalidate(dev);

	/* Fill in the super block. */
	sp->s_dev = dev;	/* readSuper() needs to know which dev */
	r = readSuper(sp);

	/* Is it recognized as a Minix filesystem? */
	if (r != OK) {
		devClose(dev);
		sp->s_dev = NO_DEV;
		return r;
	}
	
	/* Now get the inode of the file to be mounted on. */
	if (fetchName(inMsg.m_name2, inMsg.m_name2_length, M1) != OK || 
			(rip = eatPath(userPath)) == NIL_INODE) {
		devClose(dev);
		sp->s_dev = NO_DEV;
		return errCode;
	}
	
	/* It may not be busy. */
	r = OK;
	if (rip->i_count > 1)
	  r = EBUSY;

	/* It may not be special. */
	bits = rip->i_mode & I_TYPE;
	if (bits == I_BLOCK_SPECIAL || bits == I_CHAR_SPECIAL)
	  r = ENOTDIR;

	/* Get the root inode of the mounted file system. */
	rootIp = NIL_INODE;
	if (r == OK) {
		if ((rootIp = getInode(dev, ROOT_INODE)) == NIL_INODE)
		  r = errCode;
		else if (rootIp->i_mode == 0)
		  r = EINVAL;
	}

	/* File types of 'rip' and 'rootIp' may not conflit. */
	if (r == OK) {
		mdir = ((rip->i_mode & I_TYPE) == I_DIRECTORY);	/* TRUE iff dir */
		rdir = ((rootIp->i_mode & I_TYPE) == I_DIRECTORY);
		if (! mdir && rdir)
		  r = EISDIR;
	}

	/* If error, return the super block and both inodes; release the maps. */
	if (r != OK) {
		putInode(rip);
		putInode(rootIp);
		doSync();
		invalidate(dev);
		devClose(dev);
		sp->s_dev = NO_DEV;
		return r;
	}

	/* Nothing else can go wrong. Perform the mount. */
	rip->i_mount = I_MOUNT;		/* This bit says the inode is mounted on */
	sp->s_inode_mount = rip;
	sp->s_inode_super = rootIp;
	sp->s_readonly = inMsg.m_rd_only;

	return OK;
}

int unmount(dev_t dev) {
/* Unmount a file system by device number. */
	register Inode *rip;
	SuperBlock *xp, *sp;
	int count;

	/* See if the mounted device is busy. Only 1 inode using it should be
	 * open -- the root inode -- and that inode only 1 time.
	 */
	count = 0;
	for (rip = &inodes[0]; rip < &inodes[NR_INODES]; ++rip) {
		if (rip->i_count > 0 && rip->i_dev == dev)
		  count += rip->i_count;
	}
	if (count > 1)
	  return EBUSY;		/* Can't umount a busy file system. */

	/* File the super block. */
	sp = NIL_SUPER;
	for (xp = &superBlocks[0]; xp < &superBlocks[NR_SUPERS]; ++xp) {
		if (xp->s_dev == dev) {
			sp = xp;
			break;
		}
	}

	/* Sync the disk, and invalidate cache. */
	doSync();			/* Force any cached blocks out of memory */
	invalidate(dev);	/* Invalidate cache entries for this dev */
	if (sp == NIL_SUPER) 
	  return EINVAL;

	/* Close the device the file system lives on. */
	devClose(dev);

	/* Finish off the unmount. */
	sp->s_inode_mount->i_mount = NO_MOUNT;	/* Inode returns to normal */
	putInode(sp->s_inode_mount);	/* Release the inode mounted on */
	putInode(sp->s_inode_super);	/* Release the root inode of the mounted fs */
	sp->s_inode_mount = NIL_INODE;
	sp->s_inode_super = NIL_INODE;
	sp->s_dev = NO_DEV;
	return OK;
}

int doUmount() {
/* Perform the umount(special) system call. */
	dev_t dev;

	/* Only the super-user may do UMOUNT. */
	if (! superUser)
	  return EPERM;

	/* If 'special' is not for a block special file, return error. */
	if (fetchName(inMsg.m_name, inMsg.m_name_length, M3) != OK)
	  return errCode;
	if ((dev = nameToDev(userPath)) == NO_DEV)
	  return errCode;

	return unmount(dev);
}


