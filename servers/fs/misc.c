#include "fs.h"
#include "fcntl.h"
#include "unistd.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "sys/svrctl.h"
#include "file.h"
#include "fproc.h"
#include "param.h"

int doSync() {
/* Perform the sync() system call. Flush all the tables.
 * The order in which the various tables are flushed is critical. The
 * blocks must be flushed last, since rwInode() leaves its results in
 * the block cache.
 */
	register Inode *ip;
	register Buf *bp;

	/* Write all the dirty inodes to the disk. */
	for (ip = &inodes[0]; ip < &inodes[NR_INODES]; ++ip) {
		if (ip->i_count > 0 && ip->i_dirty == DIRTY)
		  rwInode(ip, WRITING);
	}

	/* Write all the dirty blocks to the disk, one drive at a time. */
	for (bp = &bufs[0]; bp < &bufs[NR_BUFS]; ++bp) {
		if (bp->b_dev != NO_DEV && bp->b_dirty == DIRTY)
		  flushAll(bp->b_dev);
	}

	return OK;	/* sync() can't fail */
}

int doFsync() {
/* Perform the fsync() system call. For now, don't be unnecessarily smart. */
	doSync();
	return OK;
}
