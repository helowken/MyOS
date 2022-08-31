#include "fs.h"
#include "unistd.h"
#include "minix/callnr.h"
#include "param.h"

int checkReadOnly(Inode *ip) {
/* Check to see if the file system on which the inode 'ip' resides is mounted
 * read only. If so, return EROFS, else return OK.
 */
	register SuperBlock *sp;

	sp = ip->i_sp;
	return sp->s_readonly ? EROFS : OK;
}

int checkForbidden(register Inode *ip, mode_t accessDesired) {
/* Given a pointer to an inode, 'ip', and the access desired, determine
 * if the access is allowed, and if not why not. The routine looks up the
 * caller's uid in the 'fprocTable'. If access is allowed, OK is returned,
 * if it is forbidden, EACCES is returned.
 */
	register Inode *oldIp = ip;
	register SuperBlock *sp;
	register mode_t bits, permBits;
	int testUid, testGid;
	int r, shift;

	if (ip->i_mount == I_MOUNT) {	/* The inode is mounted on. */
		for (sp = &superBlocks[0]; sp < &superBlocks[NR_SUPERS]; ++sp) {
			if (sp->s_inode_mount == ip) {
				ip = getInode(sp->s_dev, ROOT_INODE);
				break;
			}
		}
	}

	/* Isolate the relevant rwx bits from the mode. */
	bits = ip->i_mode;
	testUid = (callNum == ACCESS ? currFp->fp_ruid : currFp->fp_euid);
	testGid = (callNum == ACCESS ? currFp->fp_rgid : currFp->fp_egid);
	if (testUid == SU_UID) {
		/* Grant read and write permission. Grant search permission for
		 * directories. Grant execute permission (for non-directories) if
		 * and only if one of the 'X' bits is set.
		 */
		permBits = R_BIT | W_BIT;
		if ((bits & I_TYPE) == I_DIRECTORY || 
				bits & ((X_BIT << 6) | (X_BIT << 3) | X_BIT))
		  permBits |= X_BIT;
	} else {
		if (testUid == ip->i_uid)	/* Owner */
		  shift = 6;	
		else if (testGid == ip->i_gid)	/* Group */
		  shift = 3;
		else			/* Other */
		  shift = 0;
		permBits = (bits >> shift) & (R_BIT | W_BIT | X_BIT);
	}
	
	/* If access desired is not a subset of what is allowed, it is refused. */
	r = OK;
	if ((permBits | accessDesired) != permBits)
	  r = EACCES;

	/* Check to see if someone is trying to write on a file system that is 
	 * mounted read-only.
	 */
	if (r == OK && accessDesired & W_BIT)
	  r = checkReadOnly(ip);

	if (ip != oldIp)
	  putInode(ip);

	return r;
}

int doAccess() {
/* Perform the access(path, mode) system call. */
	Inode *ip;
	register int r;
	
	/* First check to see if the mode is correct. */
	if ((inMsg.m_mode & ~(R_OK | W_OK | X_OK)) != 0 && 
				inMsg.m_mode != F_OK)
	  return EINVAL;

	/* Temporarily open the file whose access is to be checked. */
	if (fetchName(inMsg.name, inMsg.name_length, M3) != OK)
	  return errCode;
	if ((ip = eatPath(userPath)) == NIL_INODE)
	  return errCode;

	/* Now check the permissions. */
	r = checkForbidden(ip, (mode_t) inMsg.m_mode);
	putInode(ip);
	return r;
}







