#include "fs.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "signal.h"
#include "stdlib.h"
#include "sys/ioc_memory.h"
#include "sys/svrctl.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "minix/keymap.h"
#include "minix/const.h"
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "super.h"
#include "inode.h"
#include "param.h"

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

static int iGetEnv(char *key, bool optional) {
	char value[64];
	int r;

	if ((r = envGetParam(key, value, sizeof(value))) != OK) {
		if (!optional)
		  printf("FS: warning, couldn't get monitor param: %d\n", r);
		return 0;
	}
	return atoi(value);
}

static void loadRam() {
/* Allocate a RAM disk with size given in the boot parameters. If a RAM disk
 * image is given, then copy the entire image device block-by-block to a RAM
 * disk with the same size as the image.
 * If the root device is not set, the RAM disk will be used as root instead.
 */
	u32_t ramSizeKB;
	dev_t imageDev;
	SuperBlock *sp;
	
	/* Get some boot environment variables. */
	rootDev = iGetEnv("rootdev", false);
	imageDev = iGetEnv("ramimagedev", false);
	ramSizeKB = iGetEnv("ramsize", false);

	/* Open the root device. */
	if (devOpen(rootDev, FS_PROC_NR, R_BIT | W_BIT) != OK)
	  panic(__FILE__, "Cannot open root device", NO_NUM);

	/* If we must initialize a ram disk, get details from the image device. */
	if (rootDev == DEV_RAM) {
		/* If we are running from CD, see if we can find it. */
		// TODO
		
		
		/* Open image device for RAM root. */
		if (devOpen(imageDev, FS_PROC_NR, R_BIT) != OK)
		  panic(__FILE__, "Cannot open RAM image device", NO_NUM);

		/* Get size of RAM disk image from the super block. */
		sp = &superBlocks[0];
		sp->s_dev = imageDev;
		if (readSuper(sp) != OK)
		  panic(__FILE__, "Bad RAM disk image FS", NO_NUM);
	}
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
		if ((s = receive(PM_PROC_NR, &msg)) != OK)
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
	buildDMap();		/* Build device table and map boot driver */
	loadRam();			/* Init RAM disk, load if it is root */
}

int main() {
/* This is the main program of the file system. The main loop consists of 
 * three major activities: getting new work, processing the work, and sending
 * the reply. This loop never terminates as long as the file system runs.
 */
	fsInit();

	return OK;
}
