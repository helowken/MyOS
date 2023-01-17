#include "fs.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "param.h"

static char modeMap[] = {R_BIT, W_BIT, R_BIT | W_BIT, 0};


static Inode *newNode(char *path, mode_t bits, zone_t z0) {
/* NewNode() is called by commonOpen(), doMknode(), and doMkdir().
 * In all cases it allocates a new inode, makes a directory entry for it on
 * the path 'path', and initializes it. It returns a pointer to the inode if
 * it can do this; otherwise it returns NIL_INODE. It always sets 'errCode'
 * to an appropriate value (OK or an error code).
 */
	register Inode *lastDirIp, *ip;
	register int r;
	char string[NAME_MAX];

	/* See if the path can be opened down to the last directory. */
	if ((lastDirIp = lastDir(path, string)) == NIL_INODE)
	  return NIL_INODE;

	/* The final directory is accessible. Get final component of the path. */
	ip = advance(lastDirIp, string);
	if (ip == NIL_INODE && errCode == ENOENT) {
		/* Last path component does not exist. Make new directory entry. */
		if ((ip = allocInode(lastDirIp->i_dev, bits)) == NIL_INODE) {
			/* Can't create new inode: out of inodes. */
			putInode(lastDirIp);
			return NIL_INODE;
		}
		
		/* Force inode to the disk before making directory entry to make
		 * the system more robust in the face of a crash: an inode with
		 * no directory entry is much better than the opposite.
		 */
		++ip->i_nlinks;
		ip->i_zone[0] = z0;		/* Major/minor device numbers */
		rwInode(ip, WRITING);	/* Force inode to disk now */

		/* New inode acquired. Try to make directory entry. */
		if ((r = searchDir(lastDirIp, string, &ip->i_num, ENTER)) != OK) {
			putInode(lastDirIp);
			--ip->i_nlinks;		/* Pity, have to free disk inode */
			ip->i_dirty = DIRTY;	/* Dirty inodes are written out */
			putInode(ip);		/* Free the inode */
			errCode = r;
			return NIL_INODE;
		}
	} else {
		/* Either last component exists, or there is some problem. */
		if (ip != NIL_INODE)
		  r = EEXIST;
		else
		  r = errCode;
	}

	/* Return the directory inode and exit. */
	putInode(lastDirIp);
	errCode = r;
	return ip;
}

static int commonOpen(register int oFlags, mode_t oMode) {
/* Common code from doCreat and doOpen. */
	register Inode *ip;
	mode_t bits;
	dev_t dev;
	Filp *filp;
	int r, exist = true;

	/* Remap the bottom two bits of oFlags. */
	bits = (mode_t) modeMap[oFlags & O_ACCMODE];

	/* See if file descriptor and filp slots are available. */
	if ((r = getFd(0, bits, &inMsg.m_fd, &filp)) != OK)
	  return r;

	/* If O_CREAT is set, try to make the file. */
	if (oFlags & O_CREAT) {
		/* Create a new inode by calling newNode(). */
		oMode = I_REGULAR | (oMode & ALL_MODES & currFp->fp_umask);
		ip = newNode(userPath, oMode, NO_ZONE);
		r = errCode;
		if (r == OK)
		  exist = false;	/* We just created the file */
		else if (r != EEXIST)
		  return r;		/* Other error */
		else {
			/* File exists, if the O_EXCL flag is set this is 
			 * an error and no need to do the following check.
			 */
			exist = !(oFlags & O_EXCL);
		}
	} else {
		/* Scan path name. */
		if ((ip = eatPath(userPath)) == NIL_INODE)
		  return errCode;
	}

	/* Claim the file descriptor and filp slot and fill them in. */
	currFp->fp_filp[inMsg.m_fd] = filp;
	filp->filp_count = 1;
	filp->filp_inode = ip;
	filp->filp_flags = oFlags;

	/* Only do the normal open code if we didn't just create the file. */
	if (exist) {
		/* Check protections. */
		if ((r = checkForbidden(ip, bits)) == OK) {
			/* Opening regular files, directories and special files differ. */
			switch (ip->i_mode & I_TYPE) {
				case I_REGULAR:
					/* Truncate regular file if O_TRUNC. */
					if (oFlags & O_TRUNC) {
						if ((r = checkForbidden(ip, W_BIT)) != OK)
						  break;
						truncate(ip);
						wipeInode(ip);
						/* Send the inode from the inode cache to the
						 * block cache, so it gets written on the next
						 * cache flush.
						 */
						rwInode(ip, WRITING);
					}
					break;
				case I_DIRECTORY:
					/* Directories may be read but not written. */
					r = bits & W_BIT ? EISDIR : OK;
					break;
				case I_CHAR_SPECIAL:
				case I_BLOCK_SPECIAL:
					/* Invoke the driver for special processing. */
					dev = (dev_t) ip->i_zone[0];
					r = devOpen(dev, who, bits | (oFlags & ~O_ACCMODE));
					break;
				case I_NAMED_PIPE:
					printf("=== TODO fs pipe commonOpen\n");
					break;
			}
		}
	}

	/* If error, release inode. */
	if (r != OK) {
		if (r == SUSPEND)
		  return r;		/* Oops, just suspended */
		currFp->fp_filp[inMsg.m_fd] = NIL_FILP;
		filp->filp_count = 0;
		putInode(ip);
		return r;
	}

	return inMsg.m_fd;
}

int doOpen() {
/* Perform the open(name, flags, ...) system call. */
	int createMode = 0;		
	int r;

	/* If O_CREAT is set, open has three parameters, otherwise two. */
	if (inMsg.m_mode & O_CREAT) {
		createMode = inMsg.m_c_mode;
		r = fetchName(inMsg.m_c_name, inMsg.m_name1_length, M1);
	} else {
		r = fetchName(inMsg.m_c_name, inMsg.m_name_length, M3);
	}

	if (r != OK)
	  return errCode;	/* Name was bad */
	r = commonOpen(inMsg.m_mode, createMode);
	return r;
}

int doMkdir() {
/* Perform the mkdir(name, mode) system call. */

	int r1, r2;			/* Status codes */
	char string[NAME_MAX];	/* Last component of the new dir's path name */
	ino_t dot, dotdot;		/* Inode numbers for . and .. */
	mode_t bits;		/* Mode bits for the new inode */
	register Inode *ip, *dirIp;

	/* Check to see if it is possible to make another link in the parent dir. */
	if (fetchName(inMsg.m_name1, inMsg.m_name1_length, M1) != OK)
	  return errCode;
	dirIp = lastDir(userPath, string);	/* Pointer to new dir's parent */
	if (dirIp->i_nlinks >= SHRT_MAX) {
		putInode(dirIp);
		return EMLINK;
	}

	/* Next make the inode. If that fails, return error code. */
	bits = I_DIRECTORY | (inMsg.m_c_mode & RWX_MODES & currFp->fp_umask);
	ip = newNode(userPath, bits, (zone_t) 0);
	if (ip == NIL_INODE || errCode == EEXIST) {
		putInode(ip);		
		putInode(dirIp);
		return errCode;
	}

	/* Get the inode numbers for . and .. to enter in the directory. */
	dotdot = dirIp->i_num;	/* Parent's inode number */
	dot = ip->i_num;	/* Inode number of the new dir itself */

	/* Now make dir entries for . and .. unless the disk is completely full. */
	/* Use dot1 and dot2, so the mode of the directory isn't important. */
	ip->i_mode = bits;	/* Set mode */
	r1 = searchDir(ip, dot1, &dot, ENTER);	/* Enter . in the new dir */
	r2 = searchDir(ip, dot2, &dotdot, ENTER);	/* Enter .. in the new dir */

	/* If both . and .. were successfully entered, increment the link counts. */
	if (r1 == OK && r2 == OK) {
		/* Normal case. It was possible to enter . and .. in the new dir. */
		++ip->i_nlinks;		/* This accounts for . */
		++dirIp->i_nlinks;	/* This accounts for .. */
		dirIp->i_dirty = DIRTY;	/* Make parent's inode as dirty */
	} else {
		/* It was not possible to enter . or .. probably disk was full. */
		searchDir(dirIp, string, (ino_t *) 0, DELETE);
		--ip->i_nlinks;		/* Undo the increment done in newNode() */
	}
	ip->i_dirty = DIRTY;	/* Either way, i_nlinks has changed */

	putInode(dirIp);		
	putInode(ip);
	return errCode;		/* newNode() always sets 'errCode' (may be OK) */
}

int doClose() {
/* Perform the close(fd) system call. */
	register Filp *filp;
	register Inode *ip;
	int rw, mode;
	dev_t dev;

	/* First locate the inode that belongs to the file descriptor. */
	if ((filp = getFilp(inMsg.m_fd)) == NIL_FILP)
	  return errCode;
	ip = filp->filp_inode;	

	if (filp->filp_count - 1 == 0 && filp->filp_mode != FILP_CLOSED) {
		/* Check to see if the file is special. */
		mode = ip->i_mode & I_TYPE;
		if (mode == I_CHAR_SPECIAL || mode == I_BLOCK_SPECIAL) {
			dev = (dev_t) ip->i_zone[0];
			if (mode == I_BLOCK_SPECIAL) {
				/* Invalidate cache entries unless special is mounted
				 * or ROOT
				 */
				printf("=== fs TODO doClose 111\n");
				//TODO
			}
			/* Do any special processing on device close. */
			devClose(dev);
		}
	}

	/* If the inode being closed is a pipe, release everyone hanging on it. */
	if (ip->i_pipe == I_PIPE) {
		rw = (filp->filp_mode & R_BIT) ? WRITE : READ;
		release(ip, rw, NR_PROCS);
	}
	
	/* If a write has been done, the inode is already marked as DIRTY. */
	if (--filp->filp_count == 0) {
		if (ip->i_pipe == I_PIPE && ip->i_count > 1) {
			/* Save the file position in the i-node in case needed later.
			 * The read and write positions are saved separately. The
			 * last 3 zones in the i-node are not used for (named) pipes.
			 */
			if (filp->filp_mode == R_BIT) 
			  ip->i_zone[NR_DIRECT_ZONES] = (zone_t) filp->filp_pos;
			else
			  ip->i_zone[NR_DIRECT_ZONES + 1] = (zone_t) filp->filp_pos;
		}
		putInode(ip);
	}

	currFp->fp_cloexec &= ~(1L << inMsg.m_fd);	/* Turn off close-on-exec bit */
	currFp->fp_filp[inMsg.m_fd] = NIL_FILP;

	/* Check to see if the file is locked. If so, release all locks. */
	if (numLocks == 0)
	  return OK;

	printf("=== fs TODO doClose 444\n");
	//TODO
	
	return OK;
}


int doLseek() {
/* Perform the lseek(fd, offset, whence) system call. */

	register Filp *filp;
	register off_t pos;

	/* Check to see if the file descriptor is valid. */
	if ((filp = getFilp(inMsg.m_ls_fd)) == NIL_FILP)
	  return errCode;

	/* No lseek on pipes. */
	if (filp->filp_inode->i_pipe == I_PIPE)
	  return ESPIPE;

	/* The value if 'whence' determines the start position to use. */
	switch (inMsg.m_whence) {
		case 0:		/* SEEK_SET */
			pos = 0;
			break;
		case 1:		/* SEEK_CUR */
			pos = filp->filp_pos;
			break;
		case 2:		/* SEEK_END */
			pos = filp->filp_inode->i_size;
			break;
		default:
			return EINVAL;
	}

	/* Check for overflow. */
	if (((long) inMsg.m_offset > 0) && ((long) (pos + inMsg.m_offset) < (long) pos))
	  return EINVAL;
	if (((long) inMsg.m_offset < 0) && ((long) (pos + inMsg.m_offset) > (long) pos))
	  return EINVAL;
	pos += inMsg.m_offset;

	if (pos != filp->filp_pos)
	  filp->filp_inode->i_seek = I_SEEK;	/* Inhibit read ahead */
	filp->filp_pos = pos;
	outMsg.m_reply_l1 = pos;	

	return OK;
}

int doCreat() {
/* Perform the creat(name, mode) system call. */
	int r;

	if (fetchName(inMsg.m_name, inMsg.m_name_length, M3) != OK)
	  return errCode;
	r = commonOpen(O_WRONLY | O_CREAT | O_TRUNC, (mode_t) inMsg.m_mode);
	return r;
}







