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
