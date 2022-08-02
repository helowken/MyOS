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
	register Filp *fp;
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
	  return ENFILE;	

	/* Now that a file descriptor has been found, look for a free filp slot. */
	for (fp = &filpTable[0]; fp < &filpTable[NR_FILPS]; ++fp) {
		if (fp->filp_count == 0) {
			fp->filp_mode = bits;
			fp->filp_pos = 0L;
			fp->filp_selectors = 0;
			fp->filp_select_ops = 0;
			fp->filp_pipe_select_ops = 0;
			fp->filp_flags = 0;
			*fpp = fp;
			return OK;
		}
	}

	/* If control passes here, the filp table must be full. Report that back. */
	return ENFILE;
}
