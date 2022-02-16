#include "fs.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "signal.h"
#include "stdlib.h"
#include "sys/ioc_memory.h"
#include "sys/srvctl.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "minix/keymap.h"
#include "minix/const.h"
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "param.h"
#include "super.h"

static void initBufPool() {
/* Initialize the buffer pool. */
	register Buf *bp;

	bufsInUse = 0;
	frontBuf = &bufs[0];
	rearBuf = &bufs[NR_BUFS - 1];

	for (bp = frontBuf; bp <= rearBuf; ++bp) {
		bp->b_block_num = NO_BLOCK;
		bp->b_dev = NO_DEV;
		bp->b_next = bp + 1;
		bp->b_prev = bp - 1;
		bp->b_hash = bp->b_next;
	}
	frontBuf->b_prev = NIL_BUF;
	rearBuf->b_hash = rearBuf->b_next = NIL_BUF;
	bufHashTable[0] = frontBuf;
}

static void buildDevMap() {
/* Initialize the table with all device <-> driver mapping. Then, map
 * the boot driver to a controller and update the devMap table to that
 * selection. The boot driver and the controller it handles are set at
 * the boot monitor.
 */
}

static void fsInit() {
/* Initialize global variables, tables, etc. */
	FProc *fp;
	Message msg;
	int s;

	/* Initialize the process table with help of the process manager messages. 
	 * Expect one message for each system process with its slot number and pid.
	 * When no more processes follow, the magic process number NONE is sent.
	 * Then, stop and synchronize with the PM.
	 */
	do {
		if ((s = receive(PM_PROC_PR, &msg)) != OK)
		  panic(__FILE__, "FS couldn't receive from PM", s);
		if (msg.PR_PROC_NR == NONE)
		  break;

		fp = &fprocTable[msg.PR_PROC_NR];
		fp->fp_pid = msg.PR_PID;
		fp->fp_ruid = (uid_t) SYS_UID;
		fp->fp_euid = (uid_t) SYS_UID;
		fp->fp_rgid = (gid_t) SYS_GID;
		fp->fp_egid = (gid_t) SYS_GID;
		fp->fp_umask = ~0;
	} while (true);		/* Continue until process NONE. */
	msg.m_type = OK;	/* Tell PM that we succeeded. */
	s = send(PM_PROC_NR, &msg);		/* Send synchronization message */

	/* The following initializations are needed to let devOpcl succeed. */
	fp = (FProc *) NULL;
	who = FS_PROC_NR;

	initBufPool();		/* Initialize buffer pool */
	buildDevMap();		/* Build device table and map boot driver */
}

int main() {
/* This is the main program of the file system. The main loop consists of 
 * three major activities: getting new work, processing the work, and sending
 * the reply. This loop never terminates as long as the file system runs.
 */
	fsInit();
}
