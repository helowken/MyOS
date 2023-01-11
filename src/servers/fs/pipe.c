#include "fs.h"
#include "fcntl.h"
#include "signal.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "sys/select.h"
#include "sys/time.h"
#include "param.h"
#include "select.h"

int doPipe() {
/* Perform the pipe(fds) system call. */
	register FProc *rfp;
	register Inode *rip;
	int r;
	Filp *fdPtr0, *fdPtr1;
	int fds[2];		/* Reply goes here */

	/* Acquire two file descriptors. */
	rfp = currFp;
	if ((r = getFd(0, R_BIT, &fds[0], &fdPtr0)) != OK)
	  return r;
	rfp->fp_filp[fds[0]] = fdPtr0;
	fdPtr0->filp_count = 1;

	if ((r = getFd(0, W_BIT, &fds[1], &fdPtr1)) != OK) {
		rfp->fp_filp[fds[0]] = NIL_FILP;
		fdPtr0->filp_count = 0;
		return r;
	}
	rfp->fp_filp[fds[1]] = fdPtr1;
	fdPtr1->filp_count = 1;

	/* Make the inode on the pipe device. */
	if ((rip = allocInode(rootDev, I_REGULAR)) == NIL_INODE) {
		rfp->fp_filp[fds[0]] = NIL_FILP;
		fdPtr0->filp_count = 0;
		rfp->fp_filp[fds[1]] = NIL_FILP;
		fdPtr1->filp_count = 0;
		return errCode;
	}

	if (checkReadOnly(rip) != OK)
	  panic(__FILE__, "pipe device is read only", NO_NUM);

	rip->i_pipe = I_PIPE;
	rip->i_mode &= ~I_REGULAR;
	rip->i_mode |= I_NAMED_PIPE;	/* Pipes and FIFOs have this bit set */
	fdPtr0->filp_inode = rip;
	fdPtr0->filp_flags = O_RDONLY;
	dupInode(rip);					/* For double usage */
	fdPtr1->filp_inode = rip;
	fdPtr1->filp_flags = O_WRONLY;
	rwInode(rip, WRITING);			/* Mark inode as allocated */

	outMsg.m_reply_i1 = fds[0];
	outMsg.m_reply_i2 = fds[1];
	rip->i_update = ATIME | CTIME | MTIME;
	return OK;
}

void suspend(int task) {
//TODO
}

void revive(int proc, int bytes) {
//TODO
}
