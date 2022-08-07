#include "fs.h"
#include "string.h"
#include "minix/callnr.h"

char dot1[2] = ".";		/* Used for searchDir to bypass the access */
char dot2[3] = "..";	/* permissions for . and .. */

static char *getName(char *oldName, char string[NAME_MAX]) {
/* Given a pointer to a path name in fs space, 'oldName', copy the next
 * component to 'string' and pad with zeros. A pointer to that part of
 * the name as yet unparsed is returned. Roughly speaking, 
 * 'getName' = 'oldName' - 'string'.
 *
 * This routine follows the standard convention that /usr/ast, /usr//ast,
 * //usr///ast and /usr/ast/ are all equivalent.
 */	
	register int c;
	register char *np, *rnp;

	np = string;		/* 'np' points to current position */
	rnp = oldName;		/* 'rnp' points to unparsed string */
	while ((c = *rnp) == '/') {
		++rnp;			/* Skip leading slashes */
	}

	/* Copy the unparsed path, 'oldName', to the array, 'string'. */
	while (rnp < &oldName[PATH_MAX] && c != '/' && c != '\0') {
		if (np < &string[NAME_MAX])
		  *np++ = c;
		c = *++rnp;		/* Advance to next character */
	}

	/* To make /usr/ast/ equivalent to /usr/ast, skip trailing slashes. */
	while (c == '/' && rnp < &oldName[PATH_MAX])
	  c = *++rnp;

	if (np < &string[NAME_MAX])
	  *np = '\0';		/* Terminate string */

	if (rnp >= &oldName[PATH_MAX]) {
		errCode = ENAMETOOLONG;
		return (char *) 0;
	}

	return rnp;
}

Inode *lastDir(char *path, char string[NAME_MAX]) {
/* Given a path, 'path', located in the fs address space, parse it as
 * far as the last directory, fetch the inode for the last directory into
 * the inode table, and return a pointer to the inode. In addition,
 * return the final component of the path in 'string'.
 * If the last directory can't be opened, return NIL_INODE and
 * the reason for failure in 'errCode'.
 */
	register Inode *ip;
	register char *newPath;
	register Inode *newIp;

	/* Is the path absolute or relative? Initialize 'ip' accordingly. */
	ip = (*path == '/' ? currFp->fp_root_dir : currFp->fp_work_dir);

	/* If dir has been removed or path is empty, return ENOENT. */
	if (ip->i_nlinks == 0 || *path == '\0') {
		errCode = ENOENT;
		return NIL_INODE;
	}

	dupInode(ip);	/* Inode will be returned with putInode */

	/* Scan the path component by component. */
	while (true) {
		/* Extract one component. */
		if ((newPath = getName(path, string)) == (char *) 0) {
			putInode(ip);	/* Bad path in user space */
			return NIL_INODE;
		}
		if (*newPath == '\0') {
			if ((ip->i_mode & I_TYPE) == I_DIRECTORY) {
				return ip;	/* Normal exit */
			} else {
				/* Last file of path prefix is not a directory */
				putInode(ip);
				errCode = ENOTDIR;
				return NIL_INODE;
			}
		}

		/* There is more path. Keep parsing. */
		newIp = advance(ip, string);
		putInode(ip);		/* ip either obsolete or irrelevant */
		if (newIp == NIL_INODE)
		  return NIL_INODE;

		/* The call to advance() succeeded. Fetch next component. */
		path = newPath;
		ip = newIp;
	}
}

Inode *advance(Inode *dirIp, char string[NAME_MAX]) {
/* Given a directory and a component of a path, look up the component in
 * the directory, find the inode, open it, and return a pointer to its inode
 * slot. If it can't be done, return NIL_INODE.
 */
	register Inode *ip;
	register SuperBlock *sp;
	Inode *ip2;
	int r;
	ino_t iNum;
	dev_t mntDev;

	/* Check for NIL_INODE */
	if (dirIp == NIL_INODE)
	  return NIL_INODE;

	/* If 'string' is empty, yield same inode straight away. */
	if (string[0] == '\0')
	  return getInode(dirIp->i_dev, dirIp->i_num);

	/* If 'string' is not present in the directory, signal error. */
	if ((r = searchDir(dirIp, string, &iNum, LOOK_UP)) != OK) {
		errCode = r;
		return NIL_INODE;
	}

	/* Don't go beyond the current root directory, unless the string is dot2. */
	if (dirIp == currFp->fp_root_dir && 
				strcmp(string, "..") == 0 && 
				string != dot2)
	  return getInode(dirIp->i_dev, dirIp->i_num);

	/* The component has been found in the directory. Get inode. */
	if ((ip = getInode(dirIp->i_dev, iNum)) == NIL_INODE)
	  return NIL_INODE;

	if (ip->i_num == ROOT_INODE) {
		if (dirIp->i_num == ROOT_INODE) {
			if (string[1] == '.') {
				for (sp = &superBlocks[1]; sp < &superBlocks[NR_SUPERS]; ++sp) {
					/* Release the root inode. Replace by the inode mounted on. */
					putInode(ip);
					mntDev = sp->s_inode_mount->i_dev;
					iNum = sp->s_inode_mount->i_num;
					ip2 = getInode(mntDev, iNum);
					ip = advance(ip2, string);
					putInode(ip2);
					break;
				}
			}
		}
	}
	if (ip == NIL_INODE)
	  return NIL_INODE;

	/* See if the inode is mounted on. If so, switch to root directory of the
	 * mounted file system. The super block provides the linkage between the
	 * inode mounted on and the root directory of the mounted file system.
	 */
	while (ip != NIL_INODE && ip->i_mount == I_MOUNT) {
		/* The inode is indeed mounted on. */
		for (sp = &superBlocks[0]; sp < &superBlocks[NR_SUPERS]; ++sp) {
			if (sp->s_inode_mount == ip) {
				/* Release the inode mounted on. Replace by the inode of the
				 * root inode of the mounted device.
				 */
				putInode(ip);
				ip = getInode(sp->s_dev, ROOT_INODE);
				break;
			}
		}
	}
	return ip;	
}

int searchDir(register Inode *dirIp, char string[NAME_MAX], ino_t *iNum, int flag) {
/* This function searches the directory whose inode is pointed to by 'dirIp':
 * if (flag == ENTER) enter 'string' in the directory with inode # '*iNum';
 * if (flag == DELETE) delete 'string' from the directory;
 * if (flag == LOOK_UP) search for 'string' and return inode # in 'iNum';
 * if (flag == IS_EMPTY) return OK if only . and .. in dir else ENOTEMPTY;
 *
 * if 'string' is dot1 or dot2, no access permissions are checked. 
 */
	register DirEntry *dp;
	register Buf *bp = NULL;
	mode_t bits;
	int r, t, i;
	bool enterHit, match, extended;
	off_t pos;
	block_t b;
	uint16_t blockSize;
	unsigned newSlots, oldSlots;
	
	/* If 'dirIp' is not a pointer to a dir inode, error. */
	if ((dirIp->i_mode & I_DIRECTORY) != I_DIRECTORY)
	  return ENOTDIR;

	r = OK;

	if (flag != IS_EMPTY) {
		bits = (flag == LOOK_UP ? X_BIT : W_BIT | X_BIT);
		/* Here use dot and dot2 to skip mode checking (so not use strcmp) */
		if (string == dot1 || string == dot2) {		
			if (flag != LOOK_UP) 
			  r = checkReadOnly(dirIp); /* Only a writable device is required. */
		} else 
		  r = checkForbidden(dirIp, bits); /* Check access permissions. */
	}
	if (r != OK)
	  return r;

	/* Step through the directory one block at a time. */
	oldSlots = (unsigned) (dirIp->i_size / DIR_ENTRY_SIZE);
	newSlots = 0;
	enterHit = false;
	match = false;
	extended = false;
	blockSize = dirIp->i_sp->s_block_size;

	for (pos = 0; pos < dirIp->i_size; pos += blockSize) {
		b = readMap(dirIp, pos);	/* Get block number */

		/* Since directories don't have holes, 'b' cannot be NO_BLOCK. */
		bp = getBlock(dirIp->i_dev, b, NORMAL);		/* Get a dir block */
		if (bp == NO_BLOCK)
		  panic(__FILE__, "getBlock returned NO_BLOCK", NO_NUM);

		/* Search a directory block. */
		for (dp = &bp->b_dirs[0]; dp < &bp->b_dirs[NR_DIR_ENTRIES(blockSize)]; ++dp) {
			if (++newSlots > oldSlots) {	/* Not found, but room left. */
				if (flag == ENTER) {
					enterHit = true;
					break;
				}
			}

			/* Match occurs if string found. */
			if (flag != ENTER && dp->d_ino != 0) {
				if (flag == IS_EMPTY) {
					/* If this test succeeds, dir is not empty. */
					if (strcmp(dp->d_name, ".") != 0 && 
							strcmp(dp->d_name, "..") != 0)
					  match = true;
				} else if (strncmp(dp->d_name, string, NAME_MAX) == 0) {
					match = true;	
				}
			}

			if (match) {
				/* LOOK_UP or DELETE found what it wanted. */
				r = OK;
				if (flag == IS_EMPTY)
				  r = ENOTEMPTY;
				else if (flag == DELETE) {
					/* Save d_ino for recovery. */
					t = NAME_MAX - sizeof(ino_t);
					*((ino_t *) &dp->d_name[t]) = dp->d_ino;
					dp->d_ino = 0;	/* Erase entry */
					bp->b_dirty = DIRTY;
					dirIp->i_update |= CTIME | MTIME;
					dirIp->i_dirty = DIRTY;
				} else 	
				  *iNum = dp->d_ino;	/* 'flag' is LOOK_UP */

				putBlock(bp, DIRECTORY_BLOCK);
				return r;
			}

			/* Check for free slot for the benefit of ENTER. */
			if (flag == ENTER && dp->d_ino == 0) {
				enterHit = true;	/* We found a free slot */
				break;
			}
		}

		/* The whole block has been searched or ENTER has a free slot. */
		if (enterHit)
		  break;	/* enterHit set if ENTER can be performed now */
		putBlock(bp, DIRECTORY_BLOCK);
	}

	/* The whole directory has now been searched. */
	if (flag != ENTER) 
	  return flag == IS_EMPTY ? OK : ENOENT;

	/* This call is for ENTER. If no free slot has been found so far, try to
	 * extend directory.
	 */
	if (!enterHit) {	/* Directory is full and no room left in last block */
		++newSlots;		/* Increase directory size by 1 entry */
		if (newSlots == 0)
		  return EFBIG;	/* Dir size limited by slot count */
		if ((bp = newBlock(dirIp, dirIp->i_size)) == NIL_BUF)
		  return errCode;
		dp = &bp->b_dirs[0];
		extended = true;
	}
	
	/* 'bp' now points to a directory block with space. 'dp' points to slot. */
	memset(dp->d_name, 0, (size_t) NAME_MAX);	/* Clear entry */
	for (i = 0; string[i] && i < NAME_MAX; ++i) {
		dp->d_name[i] = string[i];
	}
	dp->d_ino = *iNum;
	bp->b_dirty = DIRTY;
	putBlock(bp, DIRECTORY_BLOCK);

	dirIp->i_update |= CTIME | MTIME;	/* Mark mtime for update later */
	dirIp->i_dirty = DIRTY;
	if (newSlots > oldSlots) {
		dirIp->i_size = (off_t) newSlots * DIR_ENTRY_SIZE;
		/* Send the change to disk if the directory is extended. */
		if (extended)
		  rwInode(dirIp, WRITING);
	}

	return OK;
}

Inode *eatPath(char *path) {
/* Part the path 'path' and put its inode in the inode table. If not possible,
 * return NIL_INODE as function value and an error code in 'errCode'.
 */
	register Inode *dirIp, *ip;
	char string[NAME_MAX];		/* Hold 1 path component name here */

	/* First open the path down to the final directory. */
	if ((dirIp = lastDir(path, string)) == NIL_INODE) 
	  return NIL_INODE;		/* We couldn't open final directory. */

	/* The path consisting only of "/" is a special case, check for it. */
	if (string[0] == '\0')
	  return dirIp;

	/* Get final component of the path. */
	ip = advance(dirIp, string);
	putInode(dirIp);
	return ip;
}











