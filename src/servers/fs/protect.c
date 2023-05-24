#include "fs.h"
#include <unistd.h>
#include <minix/callnr.h>
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
	if (fetchName(inMsg.m_name, inMsg.m_name_length, M3) != OK)
	  return errCode;
	if ((ip = eatPath(userPath)) == NIL_INODE)
	  return errCode;

	/* Now check the permissions. */
	r = checkForbidden(ip, (mode_t) inMsg.m_mode);
	putInode(ip);
	return r;
}

int doUmask() {
/* Perform the umask(mask) system call. */
	register mode_t r;
	r = ~currFp->fp_umask;		/* Set 'r' to complement of old mask */
	currFp->fp_umask = ~(inMsg.m_mask & RWX_MODES);	
	return r;		/* Return complement of old mask */
}

int doChown() {
/* Perform the chown(name, owner, group) system call. */
	register Inode *rip;
	register int r;

	/* Temporarily open the file. */
	if (fetchName(inMsg.m_name1, inMsg.m_name1_length, M1) != OK)
	  return errCode;
	if ((rip = eatPath(userPath)) == NIL_INODE)
	  return errCode;

	/* Not permitted to change the owner of a file on a read-only file sys. */
	r = checkReadOnly(rip);
	if (r == OK) {
		/* FS is R/W. Whether call is allowed depends on ownership, etc. */
		if (superUser) {
			/* The super user can do anything. */
			rip->i_uid = inMsg.m_owner;		
		} else {
			/* Regular users can only change groups of their own files. */
			if (rip->i_uid != currFp->fp_euid ||
					rip->i_uid != inMsg.m_owner ||
					currFp->fp_egid != inMsg.m_group)
			  r = EPERM;
		}
	}
	if (r == OK) {
		rip->i_gid = inMsg.m_group;
		rip->i_mode &= ~(I_SET_UID_BIT | I_SET_GID_BIT);
		rip->i_update |= CTIME;
		rip->i_dirty = DIRTY;
	}
	putInode(rip);
	return r;
}

int doChmod() {
/* Perform the chmod(name, mode) system call. */
	register Inode *rip;
	register int r;

	/* Temporarily open the file. */
	if (fetchName(inMsg.m_name, inMsg.m_name_length, M3) != OK)
	  return errCode;
	if ((rip = eatPath(userPath)) == NIL_INODE)
	  return errCode;

	/* Only the owner or the superUser may change the mode of a file.
	 * No one may change the mode of a file on a read-only file system.
	 */
	if (rip->i_uid != currFp->fp_euid && ! superUser)
	  r = EPERM;
	else
	  r = checkReadOnly(rip);

	/* If error, return inode. */
	if (r != OK) {
		putInode(rip);
		return r;
	}

	/* Now make the change. Clear setgid bit if file is not in caller's grp */
	rip->i_mode = (rip->i_mode & ~ALL_MODES) | (inMsg.m_mode & ALL_MODES);
	if (! superUser && rip->i_gid != currFp->fp_egid)
	  rip->i_mode &= ~I_SET_GID_BIT;
	rip->i_update |= CTIME;
	rip->i_dirty = DIRTY;

	putInode(rip);
	return OK;
}



