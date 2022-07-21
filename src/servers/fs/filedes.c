#include "fs.h"

Filp *getFilp(int fd) {
/* See if 'fd' refers to a valid file descr. If so, return its Filp ptr. */
	errCode = EBADF;
	if (fd < 0 || fd >= OPEN_MAX)
	  return NIL_FILP;

	return currFp->fp_filp[fd];		/* May also be NIL_FILP */
}
