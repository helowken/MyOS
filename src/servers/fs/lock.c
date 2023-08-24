#include "fs.h"
#include <fcntl.h>
#include <unistd.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <sys/svrctl.h>
#include "param.h"

void lockRevive() {
/* Go find all the processes that are waiting for any kind of lock and
 * revive them all. The ones that are still blocked will block again when
 * they run. The others will complete. This strategy is a space-time
 * tradeoff. Figuring out exactly which ones to unblock now would take
 * extra code, and the only thing it would win would be some performance in
 * extremely race circumstances (namely, that somebody actually used
 * locking).
 *
 * See getWork() in main.c, for how to restart the call. 
 */
	int task;
	FProc *fp;

	for (fp = &fprocTable[INIT_PROC_NR + 1]; fp < &fprocTable[NR_PROCS]; ++fp) {
		task = -fp->fp_task;
		if (fp->fp_suspended == SUSPENDED && task == XLOCK) {
			revive((int) (fp - fprocTable), 0);
		}
	}
}

int lockOp(Filp *filp, int req) {
/* Perform the advisory locking required by POSIX. */
	int r, lType, i, conflict = 0, unlocking = 0;
	mode_t mode;
	off_t first, last;
	struct flock flock;
	vir_bytes userFlockAddr;
	FileLock *flckp, *flckp2, *emptyLockp;

	/* Fetch the flock structure from user space. */
	userFlockAddr = (vir_bytes) inMsg.m_buffer;	
	r = sysDataCopy(who, (vir_bytes) userFlockAddr,
			FS_PROC_NR, (vir_bytes) &flock, (phys_bytes) sizeof(flock));
	if (r != OK)
	  return EINVAL;

	/* Make some error checks. */
	lType = flock.l_type;
	mode = filp->filp_mode;
	if (lType != F_UNLCK && lType != F_RDLCK && lType != F_WRLCK)
	  return EINVAL;
	if (req == F_GETLK && lType == F_UNLCK)
	  return EINVAL;
	if ((filp->filp_inode->i_mode & I_TYPE) != I_REGULAR)
	  return EINVAL;
	if (req != F_GETLK && lType == F_RDLCK && (mode & R_BIT) == 0)
	  return EBADF;
	if (req != F_GETLK && lType == F_WRLCK && (mode & W_BIT) == 0)
	  return EBADF;

	/* Compute the first and last bytes in the lock region. */
	switch (flock.l_whence) {
		case SEEK_SET:	first = 0; break;
		case SEEK_CUR:  first = filp->filp_pos; break;
		case SEEK_END:	first = filp->filp_inode->i_size; break;
		default:		return EINVAL;
	}

	/* Check for overflow. */
	if ((long) flock.l_start > 0 && first + flock.l_start < first)
	  return EINVAL;
	if ((long) flock.l_start < 0 && first + flock.l_start > first)
	  return EINVAL;
	first += flock.l_start;
	if (flock.l_len == 0)
	  last = MAX_FILE_POS;
	else 
	  last = first + flock.l_len - 1;
	if (last < first)
	  return EINVAL;

	/* Check if this region conflicts with any existing lock. */
	emptyLockp = NIL_FILELOCK;
	for (flckp = &fileLockTable[0]; flckp < &fileLockTable[NR_LOCKS]; ++flckp) {
		if (flckp->lock_type == 0) {
			if (emptyLockp == NIL_FILELOCK)
			  emptyLockp = flckp;
			continue;	/* 0 means unused slot */
		}
		if (flckp->lock_inode != filp->filp_inode)	/* Different file */
		  continue;
		if (last < flckp->lock_first)	/* New one is in front */
		  continue;
		if (first > flckp->lock_last)	/* New one is afterwards */
		  continue;
		if (lType == F_RDLCK && flckp->lock_type == F_RDLCK)	
		  continue;
		if (lType != F_UNLCK && flckp->lock_pid == currFp->fp_pid) 
		  continue;

		/* There might be a conflict. Process it. */
		conflict = 1;
		if (req == F_GETLK)
		  break;

		/* If we are trying to set a lock, it just failed. */
		if (lType == F_RDLCK || lType == F_WRLCK) {
			if (req == F_SETLK) {
				/* For F_SETLK, just report back failure. */
				return EAGAIN;
			} else {
				/* For F_SETLKW, suspend the process. */
				suspend(XLOCK);
				return SUSPEND;
			}
		}

		/* We are clearing a lock and we found something that overlaps. */
		unlocking = 1;
		if (first <= flckp->lock_first && last >= flckp->lock_last) {
			flckp->lock_type = 0;	/* Mark slot as unused */
			--numLocks;		/* # of locks is now 1 less */
			continue;
		}

		/* Part of a locked region has been unlocked. */
		if (first <= flckp->lock_first) {
			flckp->lock_first = first + 1;
			continue;
		}
		if (last >= flckp->lock_last) {
			flckp->lock_last = first - 1;
			continue;
		}

		/* Bad luck. A lock has been split in two by unlocking the middle. */
		if (numLocks == NR_LOCKS)
		  return ENOLCK;
		for (i = 0; i < NR_LOCKS; ++i) {
			if (fileLockTable[i].lock_type == 0)
			  break;
		}
		flckp2 = &fileLockTable[i];
		flckp2->lock_type = flckp->lock_type;
		flckp2->lock_pid = flckp->lock_pid;
		flckp2->lock_inode = flckp->lock_inode;
		flckp2->lock_first = last + 1;
		flckp2->lock_last = flckp->lock_last;
		flckp->lock_last = first - 1;
		++numLocks;
	}
	if (unlocking)
	  lockRevive();

	if (req == F_GETLK) {
		if (conflict) {
			/* GETLK and conflict. Report on the conflicting lock. */
			flock.l_type = flckp->lock_type;
			flock.l_whence = SEEK_SET;
			flock.l_start = flckp->lock_first;
			flock.l_len = flckp->lock_last - flckp->lock_first + 1;
			flock.l_pid = flckp->lock_pid;
		} else {
			/* It is GETLK and there is no conflict. */
			flock.l_type = F_UNLCK;
		}

		/* Copy the flock structure back to the caller. */
		r = sysDataCopy(FS_PROC_NR, (vir_bytes) &flock,
				who, (vir_bytes) userFlockAddr, (phys_bytes) sizeof(flock));
		return r;
	} 

	if (lType == F_UNLCK)	/* Unlocked a region with no locks */
	  return OK;	

	/* There is no conflict. If space exists, store new lock in the table. */
	if (emptyLockp == NIL_FILELOCK)	/* Table is full */
	  return ENOLCK;

	emptyLockp->lock_type = lType;
	emptyLockp->lock_pid = currFp->fp_pid;
	emptyLockp->lock_inode = filp->filp_inode;
	emptyLockp->lock_first = first;
	emptyLockp->lock_last = last;
	++numLocks;

	return OK;
}





