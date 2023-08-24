#include "fs.h"

Filp *getFilp(int fd) {
/* See if 'fd' refers to a valid file descr. If so, return its Filp ptr. */
	errCode = EBADF;
	if (fd < 0 || fd >= OPEN_MAX)
	  return NIL_FILP;

	return currFp->fp_filp[fd];		/* May also be NIL_FILP */
}

int getFd(int start, mode_t bits, int *fdPtr, Filp **fpp) {
/* Look for a free file descriptor and a free filp slot. */
	register Filp *filp;
	register int i;

	*fdPtr = -1;	/* We need a way to tell if file desc found */

	/* Search the FProc fp_filp table for a free file descriptor. */
	for (i = start; i < OPEN_MAX; ++i) {
		if (currFp->fp_filp[i] == NIL_FILP) {
			*fdPtr = i;
			break;
		}
	}

	/* Check to see if a file descriptor has bee found. */
	if (*fdPtr < 0)
	  return EMFILE;	

	if (fpp == NULL)	/* Dup no need to find a filp slot */
	  return OK;

	/* Now that a file descriptor has been found, look for a free filp slot. */
	for (filp = &filpTable[0]; filp < &filpTable[NR_FILPS]; ++filp) {
		if (filp->filp_count == 0) {
			filp->filp_mode = bits;
			filp->filp_pos = 0L;
			filp->filp_selectors = 0;
			filp->filp_select_ops = 0;
			filp->filp_pipe_select_ops = 0;
			filp->filp_flags = 0;
			*fpp = filp;
			return OK;
		}
	}

	/* If control passes here, the filp table must be full. Report that back. */
	return ENFILE;
}

Filp *findFilp(register Inode *ip, mode_t bits) {
/* Find a filp slot that refers to the inode 'ip' in a way as described
 * by the mode bit 'bits'. Used for determining whether somebody is still
 * interested in either end of a pipe. Also used when opening a FIFO to
 * find partners to share a filp field with (to shared the file position).
 * Like 'getFd' it performs its job by linear search through the filp table.
 */
	register Filp *filp;

	for (filp = &filpTable[0]; filp < &filpTable[NR_FILPS]; ++filp) {
		if (filp->filp_count != 0 && 
				filp->filp_inode == ip &&
				(filp->filp_mode & bits)) 
		  return filp;
	}

	/* If control passes here, the filp wasn't there. Report that back. */
	return NIL_FILP;
}
