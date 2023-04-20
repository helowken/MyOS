#include "fs.h"
#include "sys/stat.h"
#include "string.h"
#include "minix/com.h"
#include "minix/callnr.h"
#include "param.h"

#define SAME	1000

void truncate(Inode *ip) {
/* Remove all the zones from the inode 'ip' and mark it dirty. */
	register block_t b;
	int fileType, scale, numIndZones, single, i;
	dev_t dev;
	off_t position;
	zone_t z, zoneSize, z1;
	bool wasPipe;
	Buf *bp;

	fileType = ip->i_mode & I_TYPE;		/* Check to see if file is special */
	if (fileType == I_CHAR_SPECIAL || fileType == I_BLOCK_SPECIAL)
	  return;
	dev = ip->i_dev;		/* Device on which inode resides. */
	scale = ip->i_sp->s_log_zone_size;
	zoneSize = (zone_t) ip->i_sp->s_block_size << scale;
	numIndZones = ip->i_ind_zones;

	/* Pipe can shrink, so adjust size to make sure all zones are removed. */
	wasPipe = (ip->i_pipe == I_PIPE);	/* True is this was a pipe */
	if (wasPipe)
	  ip->i_size = PIPE_SIZE(ip->i_sp->s_block_size);

	/* Step through the file a zone at a time, finding and freeing the zones. */
	for (position = 0; position < ip->i_size; position += zoneSize) {
		if ((b = readMap(ip, position)) != NO_BLOCK) {
			z = (zone_t) b >> scale;
			freeZone(dev, z);
		}
	}

	/* All the data zones have bee freed. Now free the indirect zones. */
	ip->i_dirty = DIRTY;
	if (wasPipe) {
		wipeInode(ip);		/* Clear out inode for pipes. */
		return;			/* Indirect slots contain file positions. */
	}
	single = ip->i_dir_zones;
	freeZone(dev, ip->i_zone[single]);	/* Single indirect zone */
	if ((z = ip->i_zone[single + 1]) != NO_ZONE) {
		/* Free all the single indirect zones pointed to by the double. */
		b = (block_t) z << scale;
		bp = getBlock(dev, b, NORMAL);	/* Get double indirect zone */
		for (i = 0; i < numIndZones; ++i) {
			z1 = readIndirZone(bp, i);
			freeZone(dev, z1);
		}

		/* Now free the double indirect zone itself. */
		putBlock(bp, INDIRECT_BLOCK);
		freeZone(dev, z);
	}
}

static int unlinkFile(
	Inode *dirp,			/* Parent directory of file */
	Inode *rip,				/* Inode of file, may be NIL_INODE too. */
	char fileName[NAME_MAX]	/* Name of file to be removed */
) {
/* Unlink 'fileName'; rip must be the inode of 'fileName' or NIL_INODE. */

	ino_t num;		/* Inode number */
	int r;

	/* If rip is not NIL_INODE, it is used to get faster access to the inode. */
	if (rip == NIL_INODE) {
		/* Search for file in directory and try to get its inode. */
		errCode = searchDir(dirp, fileName, &num, LOOK_UP);
		if (errCode == OK)
		  rip = getInode(dirp->i_dev, (int) num);
		if (errCode != OK || rip == NIL_INODE)
		  return errCode;
	} else {
		dupInode(rip);	/* Inode will be returned with putInode */
	}

	r = searchDir(dirp, fileName, (ino_t *) 0, DELETE);
	if (r == OK) {
		--rip->i_nlinks;	/* Entry deleted from parent's dir */
		rip->i_update |= CTIME;
		rip->i_dirty = DIRTY;
	}

	putInode(rip);
	return r;
}

static int removeDir(
	Inode *rLastDirp,		/* Parent directory */
	Inode *rip,				/* Directory to be removed */
	char dirName[NAME_MAX]	/* Name of directory to be removed */
) {
/* A directory file has to be removed. Five conditions have to met:
 *	- The file must be a directory
 *	- The directory must be empty (except for . and ..)
 *	- The final component of the path must not be . or ..
 *	- The directory must not be the root of a mounted file system
 *	- The directory must not be anybody's root/working directory
 */
	int r;
	register FProc *fp;

	/* searchDir checks that rip is a directory too. */
	if ((r = searchDir(rip, "", (ino_t *) 0, IS_EMPTY)) != OK)
	  return r;

	if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0)
	  return EINVAL;
	if (rip->i_num == ROOT_INODE)
	  return EBUSY;		/* Can't remove 'root'.*/

	for (fp = &fprocTable[INIT_PROC_NR + 1]; fp < &fprocTable[NR_PROCS]; ++fp) {
		if (fp->fp_work_dir == rip || fp->fp_root_dir == rip)
		  return EBUSY;		/* Can't remove anybody's working dir. */
	}

	/* Actually try to unlink the file; fails if parent is mode 0 etc. */
	if ((r = unlinkFile(rLastDirp, rip, dirName)) != OK)
	  return r;

	/* Unlink . and .. from the dir. The super user can link and unlink any dir,
	 * so don't make too many assumptions about them.
	 */
	unlinkFile(rip, NIL_INODE, dot1);
	unlinkFile(rip, NIL_INODE, dot2);
	return OK;
}

int doLink() {
/* Perform the link(name1, name2) system call. */
	register Inode *dirIp, *rip;
	register int r;
	char string[NAME_MAX];
	Inode *newIp;

	/* See if 'name' (file to be linekd) exists. */
	if (fetchName(inMsg.m_name1, inMsg.m_name1_length, M1) != OK)
	  return errCode;
	if ((rip = eatPath(userPath)) == NIL_INODE)
	  return errCode;

	/* Check to see if the file has maximum number of links already. */
	r = OK;
	if (rip->i_nlinks >= SHRT_MAX)
	  r = EMLINK;

	/* Only superUser may link to directories. */
	if (r == OK) {
		if ((rip->i_mode & I_TYPE) == I_DIRECTORY && ! superUser)
		  r = EPERM;
	}
	
	if (r != OK) {
		putInode(rip);
		return r;
	}

	/* Does the final directory of 'name2' exist? */
	if (fetchName(inMsg.m_name2, inMsg.m_name2_length, M1) != OK) {
		putInode(rip);
		return errCode;
	}
	if ((dirIp = lastDir(userPath, string)) == NIL_INODE) 
	  r = errCode;

	/* If 'name2' exists in full (even if no space) set 'r' to error. */
	if (r == OK) {
		if ((newIp = advance(dirIp, string)) == NIL_INODE) {
			r = errCode;
			if (r == ENOENT)
			  r = OK;
		} else {
			putInode(newIp);
			r = EEXIST;
		}
	}

	/* Check for links across devices. */
	if (r == OK) {
		if (rip->i_dev != dirIp->i_dev)
		  r = EXDEV;
	}

	/* Try to link. */
	if (r == OK) 
	  r = searchDir(dirIp, string, &rip->i_num, ENTER);
	
	/* If success, register the linking. */
	if (r == OK) {
		++rip->i_nlinks;
		rip->i_update |= CTIME;
		rip->i_dirty = DIRTY;
	}
	
	/* Done. Release both inodes. */
	putInode(rip);
	putInode(dirIp);
	return r;
}

int doUnlink() {
/* Perform he unlink(name) or rmdir(name0 system call. The code for these two
 * is almost the same. They differ only in some condition testing. Unlink()
 * may be used by the superuser to do dangerous things; rmdir() may not.
 */
	register Inode *rip;
	Inode *rLastDirp;
	int r;
	char string[NAME_MAX];

	/* Get the last directory in the path. */
	if (fetchName(inMsg.m_name, inMsg.m_name_length, M3) != OK)
	  return errCode;
	if ((rLastDirp = lastDir(userPath, string)) == NIL_INODE)
	  return errCode;

	/* The last directory exists. Does the file also exist? */
	r = OK;
	if ((rip = advance(rLastDirp, string)) == NIL_INODE)
	  r = errCode;

	/* If error, return inode. */
	if (r != OK) {
		putInode(rLastDirp);
		return r;
	}

	/* Do not remove a mount point. */
	if (rip->i_num == ROOT_INODE) {
		putInode(rLastDirp);
		putInode(rip);
		return EBUSY;
	}

	/* Now test if the call is allowed, separately for unlink() and rmdir(). */
	if (callNum == UNLINK) {
		/* Only the su may unlink directories, but the su can unlink any dir. */
		if ((rip->i_mode & I_TYPE) == I_DIRECTORY && ! superUser)
		  r = EPERM;

		/* Don't unlink a file if it is the root of a mounted file system. */
		if (rip->i_num == ROOT_INODE)
		  r = EBUSY;

		/* Actually try to unlink the file; fails if parent is mode 0 etc. */
		if (r == OK)
		  r = unlinkFile(rLastDirp, rip, string);
	} else {
		r = removeDir(rLastDirp, rip, string);	/* Call is RMDIR */
	}

	/* If unlink was possible, it has been done, otherwise it has not. */
	putInode(rip);
	putInode(rLastDirp);
	return r;
}

int doRename() {
/* Perform the rename(oldPath, newPath) system call. */

	Inode *oldDirp, *oldIp;		/* Ptrs to old dir, file inodes */
	Inode *newDirp, *newIp;		/* Ptrs to new dir, file inodes */
	Inode *newSuperDirp, *nextNewSuperDirp;
	int r = OK;					/* Error flag; initially no error */
	int isOldDir, isNewDir;		/* TRUE iff {old|new} file is dir */
	int sameParentDir;		/* TRUE iff parent dirs are the same */
	char oldName[NAME_MAX], newName[NAME_MAX];
	ino_t num;
	int r1;

	/* See if 'oldPath' (existing file) exists. Get dir and file inodes. */
	if (fetchName(inMsg.m_name1, inMsg.m_name1_length, M1) != OK)
	  return errCode;
	if ((oldDirp = lastDir(userPath, oldName)) == NIL_INODE)
	  return errCode;
	if ((oldIp = advance(oldDirp, oldName)) == NIL_INODE)
	  r = errCode;

	/* See if 'newPath' (new name) exists. Get dir and file inodes. */
	if (fetchName(inMsg.m_name2, inMsg.m_name2_length, M1) != OK)
	  r = errCode;
	if ((newDirp = lastDir(userPath, newName)) == NIL_INODE)
	  r = errCode;
	newIp = advance(newDirp, newName);	/* Not required to exist */

	if (oldIp != NIL_INODE)
	  isOldDir = (oldIp->i_mode & I_TYPE) == I_DIRECTORY;	/* TRUE iff dir */

	/* If it is ok, check for a variety of possible errors. */
	if (r == OK) {
		sameParentDir = (oldDirp == newDirp);

		/* The old inode must not be a super directory of the new last dir. */
		if (isOldDir && ! sameParentDir) {
			newSuperDirp = newDirp;
			dupInode(newSuperDirp);
			while (TRUE) {		/* May hang in a file system loop */
				if (newSuperDirp == oldIp) {
					r = EINVAL;
					break;
				}
				nextNewSuperDirp = advance(newSuperDirp, dot2);
				putInode(newSuperDirp);
				if (nextNewSuperDirp == newSuperDirp)
				  break;	/* Back at system root directory */
				newSuperDirp = nextNewSuperDirp;
				if (newSuperDirp == NIL_INODE) {
					/* Missing ".." entry. Assume the worst. */
					r = EINVAL;
					break;
				}
			}
			putInode(newSuperDirp);
		}
		
		/* The old or new name must not be . or .. */
		if (strcmp(oldName, ".") == 0 || strcmp(oldName, "..") == 0 ||
			strcmp(newName, ".") == 0 || strcmp(newName, "..") == 0)
		  r = EINVAL;

		/* Both parent directories mus be on he same device. */
		if (oldDirp->i_dev != newDirp->i_dev)
		  r = EXDEV;

		/* Parent dirs must be writable, searchable and on a writable device */
		if ((r1 = checkForbidden(oldDirp, W_BIT | X_BIT)) != OK ||
			(r1 = checkForbidden(newDirp, W_BIT | X_BIT)) != OK)
		  r = r1;

		/* Some tests apply only if the new path exists. */
		if (newIp == NIL_INODE) {
			/* Don't rename a file with a file system mounted on it. */
			if (oldIp->i_dev != oldDirp->i_dev)
			  r = EXDEV;
			if (isOldDir && newDirp->i_nlinks >= SHRT_MAX &&
				! sameParentDir && r == OK)
			  r = EMLINK;
	 	} else {
			if (oldIp == newIp)
			  r = SAME;		/* Old = new */

			/* Has the old file or new file a file system mounted on it? */
			if (oldIp->i_dev != newIp->i_dev)
			  r = EXDEV;

			isNewDir = (newIp->i_mode & I_TYPE) == I_DIRECTORY;		/* dir? */
			if (isOldDir == TRUE && isNewDir == FALSE)
			  r = ENOTDIR;
			if (isOldDir == FALSE && isNewDir == TRUE)
			  r = EISDIR;
		}
	}

	/* If a process has another root directory than the system root, we might
	 * "accidently" be moving its working directory to a place where its
	 * root directory isn't a super directory of it anymore. This can make
	 * the function chroot useless. If chroot will be used often we should
	 * probably check for it here.
	 */

	/* The rename will probably work. Only two things can go wrong now:
	 * 1. being unable to remove the new file. (when new file already exists)
	 * 2. being unable to make the new directory entry. (new file doesn't exists)
	 *		[directory has to grow by one block and cannot because the disk
	 *		is completely full].
	 */
	if (r == OK) {
		if (newIp != NIL_INODE) {
			/* There is already an entry for 'new'. Try to remove it. */
			if (isOldDir)
			  r = removeDir(newDirp, newIp, newName);
			else
			  r = unlinkFile(newDirp, newIp, newName);
		}
		/* If r is OK, the rename will succeed, while there is now an
		 * unused entry in the new parent directory.
		 */
	}

	if (r == OK) {
		/* If the new name will be in the same parent directory as the old one,
		 * first remove the old name to free an entry for the new name,
		 * otherwise first try to create the new name entry to make sure
		 * the rename will succeed.
		 */
		num = oldIp->i_num;		/* Inode number of old file */

		if (sameParentDir) {
			r = searchDir(oldDirp, oldName, (ino_t *) 0, DELETE);
			if (r == OK)
			  searchDir(oldDirp, newName, &num, ENTER);
		} else {
			r = searchDir(newDirp, newName, &num, ENTER);
			if (r == OK)
			  searchDir(oldDirp, oldName, (ino_t *) 0, DELETE);
		}
	}
	/* If r is OK, the ctime and mtime of oldDirp and newDirp have been marked
	 * for update in searchDir.
	 */

	if (r == OK && isOldDir && ! sameParentDir) {
		/* Update the .. entry in the directory (still points to oldDirp). */
		num = newDirp->i_num;
		unlinkFile(oldIp, NIL_INODE, dot2);
		if (searchDir(oldIp, dot2, &num, ENTER) == OK) {
			/* New link created. */
			++newDirp->i_nlinks;
			newDirp->i_dirty = DIRTY;
		}
	}

	/* Release the inodes. */
	putInode(oldDirp);
	putInode(oldIp);
	putInode(newDirp);
	putInode(newIp);

	return r == SAME ? OK : r;
}



