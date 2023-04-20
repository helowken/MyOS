#include "fs.h"
#include "fcntl.h"
#include "minix/com.h"
#include "select.h"
#include "sys/time.h"
#include "sys/select.h"

/* Max number of simultaneously pending select() calls */
#define MAX_SELECTS	25

typedef struct {
	FProc *requestor;	/* Slot is free iff this is NULL */
	int reqProcNum;
	fd_set readFds, writeFds, errorFds;
	fd_set readyReadFds, readyWriteFds, readyErrorFds;
	fd_set *virReadFds, *virWriteFds, *virErrorFds;
	Filp *filps[FD_SETSIZE];
	int type[FD_SETSIZE];
	int nFds, nReadyFds;
	clock_t expiry;
	Timer timer;	/* If expiry > 0 */
} SelectEntry;
static SelectEntry selectTab[MAX_SELECTS];

#define SEL_FD_FILE	0
#define SEL_FD_PIPE	1
#define SEL_FD_TTY	2
#define SEL_FD_INET	3
#define SEL_FD_LOG	4
#define SEL_FDS		5

static int selectRequestFile(Filp *filp, int *ops, int block);
static int selectMatchFile(Filp *filp);
static int selectRequestGeneral(Filp *filp, int *ops, int block);
static int selectRequestPipe(Filp *filp, int *ops, int block);
static int selectMatchPipe(Filp *filp);

typedef struct {
	int (*selectRequest)(Filp *, int *ops, int block);
	int (*selectMatch)(Filp *);
	int selectMajor;
} FdType;
static FdType fdTypes[SEL_FDS] = {
		/* SEL_FD_FILE */
	{ selectRequestFile, selectMatchFile, 0 },
		/* SEL_FD_TTY (also PTY) */
	{ selectRequestGeneral, NULL, TTY_MAJOR },
		/* SEL_FD_INET */
	{ selectRequestGeneral, NULL, INET_MAJOR },
		/* SEL_FD_PIPE (pipe(2) pipes and FS FIFOs) */
	{ selectRequestPipe, selectMatchPipe, 0 },
		/* SEL_FD_LOG (/dev/klog) */
	{ selectRequestGeneral, NULL, LOG_MAJOR }
};

void initSelect() {
	int i;

	for (i = 0; i < MAX_SELECTS; ++i) {
		fsInitTimer(&selectTab[i].timer);
	}
}

static int selectRequestFile(Filp *filp, int *ops, int block) {
	/* Output *ops is input *ops */
	return SEL_OK;
}

static int selectRequestGeneral(Filp *filp, int *ops, int block) {
	int reqOps = *ops;

	if (block)
	  reqOps |= SEL_NOTIFY;
	*ops = devIO(DEV_SELECT, filp->filp_inode->i_zone[0], reqOps, 
					NULL, 0, 0, 0);
	if (*ops < 0)
	  return SEL_ERR;
	return SEL_OK;
}

static int selectRequestPipe(Filp *filp, int *ops, int block) {
	int origOps, r = 0, err, bytesCanWrite;

	origOps = *ops;
	if ((origOps & SEL_RD)) {
		if ((err = pipeCheck(filp->filp_inode, READING, 0, 1, 
					filp->filp_pos, &bytesCanWrite, 1)) != SUSPEND)
		  r |= SEL_RD;
		if (err < 0 && err != SUSPEND && (origOps & SEL_ERR))
		  r |= SEL_ERR;
	} 
	if ((origOps & SEL_WR)) {
		if ((err = pipeCheck(filp->filp_inode, WRITING, 0, 1, 
					filp->filp_pos, &bytesCanWrite, 1)) != SUSPEND)
		  r |= SEL_WR;
		if (err < 0 && err != SUSPEND && (origOps & SEL_ERR))
		  r |= SEL_ERR;
	} 

	*ops = r;

	if (! r && block)
	  filp->filp_pipe_select_ops |= origOps;

	return SEL_OK;
}

static int tab2Ops(int fd, SelectEntry *e) {
	return (FD_ISSET(fd, &e->readFds) ? SEL_RD : 0) |
		(FD_ISSET(fd, &e->writeFds) ? SEL_WR : 0) |
		(FD_ISSET(fd, &e->errorFds) ? SEL_ERR : 0);
}

static void ops2Tab(int ops, int fd, SelectEntry *e) {
	if ((ops & SEL_RD) && e->virReadFds && 
			FD_ISSET(fd, &e->readFds) && 
			! FD_ISSET(fd, &e->readyReadFds)) {
		FD_SET(fd, &e->readyReadFds);
		++e->nReadyFds;
	}
	if ((ops & SEL_WR) && e->virWriteFds &&
			FD_ISSET(fd, &e->writeFds) &&
			! FD_ISSET(fd, &e->readyWriteFds)) {
		FD_SET(fd, &e->readyWriteFds);
		++e->nReadyFds;
	}
	if ((ops & SEL_ERR) && e->virErrorFds &&
			FD_ISSET(fd, &e->errorFds) &&
			! FD_ISSET(fd, &e->readyErrorFds)) {
		FD_SET(fd, &e->readyErrorFds);
		++e->nReadyFds;
	}
}

static int selectReevaluate(Filp *filp) {
	int s, remainOps = 0, fd;

	if (! filp) {
		printf("fs: select: reevaluate NULL filp\n");
		return 0;
	}

	for (s = 0; s < MAX_SELECTS; ++s) {
		if (! selectTab[s].requestor)
		  continue;
		for (fd = 0; fd < selectTab[s].nFds; ++fd) {
			if (filp == selectTab[s].filps[fd]) 
			  remainOps |= tab2Ops(fd, &selectTab[s]);
		}
	}

	/* If there are any select()s open that want any operations on
	 * this fd that haven't been satisfied by this callback, then we're
	 * still in the market for it.
	 */
	filp->filp_select_ops = remainOps;

	return remainOps;
}

static void selectCancelAll(SelectEntry *e) {
	int fd;
	Filp *fp;

	for (fd = 0; fd < e->nFds; ++fd) {
		fp = e->filps[fd];
		if (! fp) 
		  continue;
		if (fp->filp_selectors < 1) 
		  continue;
		--fp->filp_selectors;
		e->filps[fd] = NULL;
		selectReevaluate(fp);
	}

	if (e->expiry > 0) {
		fsCancelTimer(&e->timer);	
		e->expiry = 0;
	}
}

static int selectMatchFile(Filp *filp) {
	return filp && 
		filp->filp_inode &&
		(filp->filp_inode->i_mode & I_REGULAR);
}

static int selectMatchPipe(Filp *filp) {
	/* Recognize either pipe or named pipe (FIFO) */
	if (filp && 
			filp->filp_inode && 
			(filp->filp_inode->i_mode & I_NAMED_PIPE))
	  return 1;
	return 0;
}

static int selectMajorMatch(int matchMajor, Filp *filp) {
	int major;

	if (! (filp && 
			filp->filp_inode &&
			(filp->filp_inode->i_mode & I_TYPE) == I_CHAR_SPECIAL))
	  return 0;

	major = MAJOR_DEV(filp->filp_inode->i_zone[0]);
	if (major == matchMajor)
	  return 1;
	return 0;
}

static void copyFdsets(SelectEntry *e) {
	if (e->virReadFds) 
	  sysVirCopy(SELF, D, (vir_bytes) &e->readyReadFds,
		e->reqProcNum, D, (vir_bytes) e->virReadFds, sizeof(fd_set));
	if (e->virWriteFds)
	  sysVirCopy(SELF, D, (vir_bytes) &e->readyWriteFds,
		e->reqProcNum, D, (vir_bytes) e->virWriteFds, sizeof(fd_set));
	if (e->virErrorFds)
	  sysVirCopy(SELF, D, (vir_bytes) &e->readyErrorFds,
		e->reqProcNum, D, (vir_bytes) e->virErrorFds, sizeof(fd_set));
}

static void selectWakeup(SelectEntry *e) {
	/* Open Group:
	 * "upon successful completion, the pselect() and select()
	 * functions shall return the total number of bits
	 * set in the bit masks."
	 */
	revive(e->reqProcNum, e->nReadyFds);
}

int selectCallback(Filp *filp, int ops) {
	int s, fd;

	/* We are being notified that file pointer filp is available for
	 * operations 'ops'. We must re-register the select for
	 * operations that we are still interested in, if any.
	 */

	for (s = 0; s < MAX_SELECTS; ++s) {
		int wakeHim = 0;

		if (! selectTab[s].requestor) 
		  continue;

		for (fd = 0; fd < selectTab[s].nFds; ++fd) {
			if (! selectTab[s].filps[fd])
			  continue;
			if (selectTab[s].filps[fd] == filp) {
				int thisWantOps;

				thisWantOps = tab2Ops(fd, &selectTab[s]);
				if (thisWantOps & ops) {
					/* This select() has been satisfied. */
					ops2Tab(ops, fd, &selectTab[s]);
					wakeHim = 1;
				}
			}
		}
		if (wakeHim) {
			copyFdsets(&selectTab[s]);
			selectCancelAll(&selectTab[s]);
			selectTab[s].requestor = NULL;
			selectWakeup(&selectTab[s]);
		}
	}
	return 0;
}

static void selectTimeoutCheck(Timer *timer) {
	int s;

	s = timerArg(timer)->ta_int;

	if (s < 0 || s >= MAX_SELECTS)
	  return;

	if (! selectTab[s].requestor)
	  return;

	if (selectTab[s].expiry <= 0) 
	  return;

	selectTab[s].expiry = 0;
	copyFdsets(&selectTab[s]);
	selectCancelAll(&selectTab[s]);
	selectTab[s].requestor = NULL;
	selectWakeup(&selectTab[s]);
}

int selectNotified(int major, int minor, int selectedOps) {
	int s, f, t;

	for (t = 0; t < SEL_FDS; ++t) {
		if (! fdTypes[t].selectMatch && fdTypes[t].selectMajor == major)
		  break;
	}
	if (t >= SEL_FDS)
	  return OK;

	/* We have a select callback from amjor device no.d,
	 * which corresponds to our select type t.
	 */
	for (s = 0; s < MAX_SELECTS; ++s) {
		int selMinor, ops;

		if (! selectTab[s].requestor)
		  continue;

		for (f = 0; f < selectTab[s].nFds; ++f) {
			if (! selectTab[s].filps[f] ||
				! selectMajorMatch(major, selectTab[s].filps[f]))
			  continue;

			ops = tab2Ops(f, &selectTab[s]);
			selMinor = MINOR_DEV(selectTab[s].filps[f]->filp_inode->i_zone[0]);
			if (selMinor == minor && (selectedOps & ops)) 
			  selectCallback(selectTab[s].filps[f], (selectedOps & ops));
		}
	}

	return OK;
}	

int doSelect(void) {
	int r, nfds, isTimeout = 1, nonZeroTimeout = 0,
		fd, s, block = 0;
	struct timeval timeout;

	nfds = inMsg.SEL_NFDS;
	if (nfds < 0 || nfds > FD_SETSIZE)
	  return EINVAL;

	for (s = 0; s < MAX_SELECTS; ++s) {
		if (! selectTab[s].requestor)
		  break;
	}
	if (s >= MAX_SELECTS)
	  return ENOSPC;

	selectTab[s].reqProcNum = who;
	selectTab[s].nFds = 0;
	selectTab[s].nReadyFds = 0;
	memset(selectTab[s].filps, 0, sizeof(selectTab[s].filps));
	
	/* Defaults */
	FD_ZERO(&selectTab[s].readFds);
	FD_ZERO(&selectTab[s].writeFds);
	FD_ZERO(&selectTab[s].errorFds);
	FD_ZERO(&selectTab[s].readyReadFds);
	FD_ZERO(&selectTab[s].readyWriteFds);
	FD_ZERO(&selectTab[s].readyErrorFds);

	selectTab[s].virReadFds = (fd_set *) inMsg.SEL_READFDS;
	selectTab[s].virWriteFds = (fd_set *) inMsg.SEL_WRITEFDS;
	selectTab[s].virErrorFds = (fd_set *) inMsg.SEL_ERRORFDS;

	/* Copy args */
	if (selectTab[s].virReadFds && 
			(r = sysVirCopy(who, D, (vir_bytes) selectTab[s].virReadFds, 
			SELF, D, (vir_bytes) &selectTab[s].readFds, sizeof(fd_set))) != OK)
	  return r;
	if (selectTab[s].virWriteFds && 
			(r = sysVirCopy(who, D, (vir_bytes) selectTab[s].virWriteFds, 
			SELF, D, (vir_bytes) &selectTab[s].writeFds, sizeof(fd_set))) != OK)
	  return r;
	if (selectTab[s].virErrorFds && 
			(r = sysVirCopy(who, D, (vir_bytes) selectTab[s].virErrorFds, 
			SELF, D, (vir_bytes) &selectTab[s].errorFds, sizeof(fd_set))) != OK)
	  return r;

	if (! inMsg.SEL_TIMEOUT)
	  isTimeout = nonZeroTimeout = 0;
	else if ((r = sysVirCopy(who, D, (vir_bytes) inMsg.SEL_TIMEOUT,
				SELF, D, (vir_bytes) &timeout, sizeof(timeout))) != OK)
	  return r;

	/* No nonsense in the timeval please. */
	if (isTimeout && (timeout.tv_sec < 0 || timeout.tv_usec < 0))
	  return EINVAL;

	/* If isTimeout is 0, we block forever. Otherwise, if nonZeroTimeout is 0,
	 * we do a poll (don't block). Otherwise we block up to the specified
	 * time interval.
	 */
	if (isTimeout && (timeout.tv_sec > 0 || timeout.tv_usec > 0))
	  nonZeroTimeout = 1;

	if (nonZeroTimeout || ! isTimeout)
	  block = 1;
	else
	  block = 0;	/* Timeout set as (0,0) - this effects a poll */

	/* No timeout set (yet) */
	selectTab[s].expiry = 0;

	for (fd = 0; fd < nfds; ++fd) {
		int ops, t, type = -1, r;
		Filp *filp;

		if (! (ops = tab2Ops(fd, &selectTab[s])))
		  continue;
		if (! (filp = selectTab[s].filps[fd] = getFilp(fd))) {
			selectCancelAll(&selectTab[s]);
			return EBADF;
		}

		for (t = 0; t < SEL_FDS; ++t) {
			if (fdTypes[t].selectMatch) {
				if (fdTypes[t].selectMatch(filp)) {
					if (type != -1)
					  printf("select: double match\n");
					type = t;
				}
			} else if (selectMajorMatch(fdTypes[t].selectMajor, filp)) {
				type = t;
			}
		}

		/* Open Group:
		 * "The pselect() and select() functions shall support
		 * regular files, terminal and pseudo-terminal devices,
		 * STREAMS-based files, FIFOs, pipes, and sockets. The
		 * behavior of pselect() and select() on file descriptors
		 * that refer to other types of file is unspecified."
		 *
		 * If all types are implemented, then this is another
		 * type of file and we get to do whatever we want.
		 */
		if (type == -1)
		  return EBADF;

		selectTab[s].type[fd] = type;

		if ((selectTab[s].filps[fd]->filp_select_ops & ops) != ops) {
			int wantOps;
			/* Request the select on this fd. */
			wantOps = (selectTab[s].filps[fd]->filp_select_ops |= ops);

			if ((r = fdTypes[type].selectRequest(filp, &wantOps, 
								block)) != SEL_OK) {
				/* Error or bogus return code. */
				selectCancelAll(&selectTab[s]);
				printf("select: selectRequest returned error\n");
				return EINVAL;
			}
			if (wantOps) {
				if (wantOps & ops) {
					/* Operations that were just requested are 
					 * ready to go right away
					 */
					ops2Tab(wantOps, fd, &selectTab[s]);
				}
				/* If there are any other select()s blocking on 
				 * these operations of this filp, they can be
				 * awoken too.
				 *
				 * Note: selectTab[s].requestor has not been set, so 
				 * it will not be notified.
				 */
				selectCallback(filp, wantOps);	//TODO or ops?
			}
		}

		selectTab[s].nFds = fd + 1;
		++selectTab[s].filps[fd]->filp_selectors;
	}

	if (selectTab[s].nReadyFds > 0 || ! block) {
		/* fd's were found that were ready to go right away, and/or
		 * we were instructed not to block at all. Must return 
		 * immediately.
		 */
		copyFdsets(&selectTab[s]);
		selectCancelAll(&selectTab[s]);
		selectTab[s].requestor = NULL;

		/* Open Group:
		 * "Upon successful completion, the pselect() and select()
		 * functions shall return the total number of bites
		 * set in the bit masks."
		 */
		return selectTab[s].nReadyFds;
	}

	/* Convert timeval to ticks and set the timer. If it fails, undo
	 * all, return error.
	 */
	if (isTimeout) {
		int ticks;
		/* Open Group:
		 * "If the requested timeout interval requires a finer
		 * granularity than the implementation supports, the
		 * actual timeout interval shall be rounded up to the next
		 * supported value."
		 */
#define USEC_PER_SEC	1000000
		while (timeout.tv_usec >= USEC_PER_SEC) {
			/* This is to avoid overflow with *HZ below */
			timeout.tv_usec -= USEC_PER_SEC;
			++timeout.tv_sec;
		}
		ticks = timeout.tv_sec * HZ + 
			(timeout.tv_usec * HZ + USEC_PER_SEC - 1) / USEC_PER_SEC;
		selectTab[s].expiry = ticks;
		fsSetTimer(&selectTab[s].timer, ticks, selectTimeoutCheck, s);
	}

	/* If we're blocking, the table entry is now valid. */
	selectTab[s].requestor = currFp;

	/* Process now blocked */
	suspend(XSELECT);
	return SUSPEND;
}



