#include "fs.h"
#include "sys/stat.h"
#include "minix/com.h"
#include "param.h"

static int statInode(Inode *ip, Filp *fp, char *userAddr) {
/* Common code for stat and fstat system calls. */
	struct stat sb;
	mode_t mode;
	int r, s;

	/* Update the atime, ctime, and mtime fields in the inode, if need be. */
	if (ip->i_update)
	  updateTimes(ip);
	
	mode = ip->i_mode & I_TYPE;

	/* true iff special */
	s = (mode == I_CHAR_SPECIAL || mode == I_BLOCK_SPECIAL);
	
	sb.st_dev = ip->i_dev;
	sb.st_ino = ip->i_num;
	sb.st_mode = ip->i_mode;
	sb.st_nlink = ip->i_nlinks;
	sb.st_uid = ip->i_uid;
	sb.st_gid = ip->i_gid;
	sb.st_rdev = (dev_t) (s ? ip->i_zone[0] : NO_DEV);
	sb.st_size = ip->i_size;

	if (ip->i_pipe == I_PIPE) {
		sb.st_mode &= ~I_REGULAR;	/* Wipe out I_REGULAR bit for pipes */
		if (fp != NIL_FILP && fp->filp_mode & R_BIT)
		  sb.st_size -= fp->filp_pos;
	}

	sb.st_atime = ip->i_atime;
	sb.st_mtime = ip->i_mtime;
	sb.st_ctime = ip->i_ctime;

	/* Copy the struct to user space. */
	r = sysDataCopy(FS_PROC_NR, (vir_bytes) &sb, 
			who, (vir_bytes) userAddr, (phys_bytes) sizeof(sb));
	return r;
}

int doFstat() {
/* Perform the fstat(fd, buf) system call. */
	register Filp *fp;

	/* Is the file descriptor valid? */
	if ((fp = getFilp(inMsg.m_fd)) == NIL_FILP)
	  return errCode;

	return statInode(fp->filp_inode, fp, inMsg.buffer);
}

static int changeIntoDir(Inode **iip, Inode *rip) {
	register int r;

	/* It must be a directory and also be searchable. */
	if ((rip->i_mode & I_TYPE) != I_DIRECTORY)
	  r = ENOTDIR;
	else 
	  r = checkForbidden(rip, X_BIT);	/* Check if dir is searchable */

	/* If error, return inode. */
	if (r != OK) {
		putInode(rip);
		return r;
	}

	/* Everything is OK. Make the change. */
	putInode(*iip);		/* Release the old directory */
	*iip = rip;		/* Acquire the new one */
	return OK;
}

static int changeDir(Inode **iip, char *pathName, int len) {
/* Do the actual work for chdir() and chroot(). */
	Inode *rip;

	/* Try to open the new directory. */
	if (fetchName(pathName, len, M3) != OK)
	  return errCode;
	if ((rip = eatPath(userPath)) == NIL_INODE)
	  return errCode;
	return changeIntoDir(iip, rip);
}

int doChdir() {
/* Change directory. This function is also called by MM to simulate a chdir
 * in order to do EXEC, etc. It also changes the root directory, the uids and
 * gids, and the umask.
 */
	register FProc *rfp;
	
	if (who == PM_PROC_NR) {
		rfp = &fprocTable[inMsg.slot1];
		putInode(currFp->fp_root_dir);
		currFp->fp_root_dir = rfp->fp_root_dir;
		dupInode(currFp->fp_root_dir);

		putInode(currFp->fp_work_dir);
		currFp->fp_work_dir = rfp->fp_work_dir;
		dupInode(currFp->fp_work_dir);

		/* MM uses access() to check permissions. To make this work, pretend
		 * that the user's real ids are the same as the user's effective ids.
		 * FS calls other than access() do not use the real ids, so are not
		 * affected.
		 */
		currFp->fp_ruid = 
		currFp->fp_euid = rfp->fp_euid;
		currFp->fp_rgid = 
		currFp->fp_egid = rfp->fp_egid;
		currFp->fp_umask = rfp->fp_umask;
		return OK;
	}

	/* Perform the chdir(name0 system call. */
	return changeDir(&currFp->fp_work_dir, inMsg.name, inMsg.name_length);
}











