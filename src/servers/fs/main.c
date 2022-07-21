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
#include "param.h"

#define KB	1024

static void initBufPool() {
/* Initialize the buffer pool. */
	register Buf *bp;

	bufsInUse = 0;
	frontBp = &bufs[0];
	rearBp = &bufs[NR_BUFS - 1];

	for (bp = frontBp; bp <= rearBp; ++bp) {
		bp->b_block_num = NO_BLOCK;
		bp->b_dev = NO_DEV;
		bp->b_next = bp + 1;
		bp->b_prev = bp - 1;
		bp->b_hash_next = bp->b_next;
	}
	frontBp->b_prev = NIL_BUF;
	rearBp->b_next = NIL_BUF;
	rearBp->b_hash_next = NIL_BUF;
	bufHashTable[0] = frontBp;
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
	u32_t imgBlocks, ramSizeInKB;
	dev_t imageDev;
	static char sbBuf[SUPER_BLOCK_BYTES];
	SuperBlock *imgSp, *ramSp;
	int imgBlockSize, ramBlockSize, factor;
	int s;
	block_t imgBlockNum, ramBlockNum;
	Buf *imgBp, *ramBp;
	
	/* Get some boot environment variables. */
	rootDev = iGetEnv("rootdev", false);
	imageDev = iGetEnv("ramimagedev", false);
	ramSizeInKB = iGetEnv("ramsize", false);

	/* Open the root device. */
	if (devOpen(rootDev, FS_PROC_NR, R_BIT | W_BIT) != OK)
	  panic(__FILE__, "Cannot open root device", NO_NUM);

	/* If we must initialize a ram disk, get details from the image device. */
	if (rootDev == DEV_RAM) {
		u32_t fsMax;
		/* If we are running from CD, see if we can find it. */
		// TODO
		
		
		/* Open image device for RAM root. */
		if (devOpen(imageDev, FS_PROC_NR, R_BIT) != OK)
		  panic(__FILE__, "Cannot open RAM image device", NO_NUM);

		/* Get size of RAM disk image from the super block. */
		imgSp = &superBlocks[0];
		imgSp->s_dev = imageDev;
		if (readSuper(imgSp) != OK)
		  panic(__FILE__, "Bad RAM disk image FS", NO_NUM);

		imgBlocks = imgSp->s_zones << imgSp->s_log_zone_size;	/* # blocks on image dev */

		/* Stretch the RAM disk file system to the boot parameters size, but
		 * no further than the last zone bit map block allows.
		 */
		if (ramSizeInKB * KB < imgBlocks * imgSp->s_block_size) 
		  ramSizeInKB = imgBlocks * imgSp->s_block_size / KB;

		/* Total data zones */
		fsMax = (u32_t) imgSp->s_zmap_blocks * imgSp->s_block_size * CHAR_BIT;

		/* Total zones = total data zones + zone offset
		 * Total blocks = total zones * blocks per zone 
		 */
		fsMax = (fsMax + (imgSp->s_first_data_zone - 1)) << imgSp->s_log_zone_size;
		if (ramSizeInKB * KB > fsMax * imgSp->s_block_size)
		  ramSizeInKB = fsMax * imgSp->s_block_size / KB;
	}

	/* Tell RAM driver how big the RAM disk must be. */
	outMsg.m_type = DEV_IOCTL;
	outMsg.PROC_NR = FS_PROC_NR;
	outMsg.DEVICE = RAM_DEV;
	outMsg.REQUEST = MEM_IOC_RAM_SIZE;
	outMsg.POSITION = ramSizeInKB * KB;
	if ((s = sendRec(MEM_PROC_NR, &outMsg)) != OK) {
	    panic("FS", "sendRec from MEM failed", s);
	} else if (outMsg.REP_STATUS != OK) {
		/* Report and continue, unless RAM disk is required as ROOT FS. */
		if (rootDev != DEV_RAM) 
		  report("FS", "can't set RAM disk size", outMsg.REP_STATUS);
		else
		  panic(__FILE__, "can't set RAM disk size", outMsg.REP_STATUS);
	}

	/* See if we must load the RAM disk image, otherwise return. */
	if (rootDev != DEV_RAM)
	  return;

	/* Copy the blocks one at a time from the image to the RAM disk. */
	printf("Loading RAM disk onto /dev/ram:\33[23CLoaded:    0 KB");

	inodes[0].i_mode = I_BLOCK_SPECIAL;	
	inodes[0].i_size = LONG_MAX;
	inodes[0].i_dev = imageDev;
	inodes[0].i_zone[0] = imageDev;

	ramBlockSize = getBlockSize(DEV_RAM);
	imgBlockSize = getBlockSize(imageDev);

	/* The image block size has to be multiple of the ram block
	 * size to make copying easier.
	 */
	if (imgBlockSize % ramBlockSize) { 
		printf("\nram block size: %d image block size: %d\n",
			ramBlockSize, imgBlockSize);
		panic(__FILE__, "the image block size must be a multiple of "
			"the ram disk block size", NO_NUM);
	}

	/* Loading blocks from image device. */
	for (imgBlockNum = 0; imgBlockNum < (block_t) imgBlocks; ++imgBlockNum) {
		imgBp = doReadAhead(&inodes[0], imgBlockNum, 
				(off_t) imgBlockSize * imgBlockNum, imgBlockSize);
		factor = imgBlockSize / ramBlockSize;
		for (ramBlockNum = 0; ramBlockNum < factor; ++ramBlockNum) {
			ramBp = getBlock(rootDev, 
						imgBlockNum * factor + ramBlockNum, 
						NO_READ);
			memcpy(ramBp->b_data, 
					imgBp->b_data + ramBlockNum * ramBlockSize, 
					(size_t) ramBlockSize);
			ramBp->b_dirty = DIRTY;
			putBlock(ramBp, FULL_DATA_BLOCK);
		}
		putBlock(imgBp, FULL_DATA_BLOCK);
		if (imgBlockNum % 11 == 0)
		  printf("\b\b\b\b\b\b\b\b\b%6ld KB", ((long) imgBlockNum * imgBlockSize) / KB);
	}

	/* Commit changes to RAM so devIO will see it. */
	doSync();

	printf("\rRAM disk of %u KB loaded onto /dev/ram.", (unsigned) ramSizeInKB);
	printf(" Using RAM disk as root FS.\n");

	/* Invalidate and close the image device. */
	invalidate(imageDev);
	devClose(imageDev);

	/* Resize the RAM disk root file system. */
	if (devIO(DEV_READ, rootDev, FS_PROC_NR, sbBuf, SUPER_OFFSET_BYTES, 
				SUPER_BLOCK_BYTES, 0) != SUPER_BLOCK_BYTES) {
		printf("WARNING: ram disk read for resizing failed\n");
	}
	ramSp = (SuperBlock *) sbBuf;
	ramSp->s_zones = (ramSizeInKB * KB / ramSp->s_block_size) >> 
						ramSp->s_log_zone_size;
	if (devIO(DEV_WRITE, rootDev, FS_PROC_NR, sbBuf, SUPER_OFFSET_BYTES,
				SUPER_BLOCK_BYTES, 0) != SUPER_BLOCK_BYTES) {
		printf("WARNING: ram disk write for resizing failed\n");
	}
}

static void loadSuper(dev_t dev) {
	int bad;
	register SuperBlock *sp;
	register Inode *ip;

	/* Initialize the superblock table. */
	for (sp = &superBlocks[0]; sp < &superBlocks[NR_SUPERS]; ++sp) {
		sp->s_dev = NO_DEV;
	}

	/* Read in superblock for the root file system. */
	sp = &superBlocks[0];
	sp->s_dev = dev;

	/* Check superblock for consistency. */
	bad = readSuper(sp) != OK;
	if (!bad) {
		ip = getInode(dev, ROOT_INODE);		/* Inode for root dir */
		if (ip == NIL_INODE || (ip->i_mode & I_TYPE) != I_DIRECTORY)
					//TODO || ip->i_nlinks < 3)
		  ++bad;
	}
	if (bad) 
	  panic(__FILE__, "Invalid root file system", NO_NUM);

	sp->s_inode_mount = ip;
	dupInode(ip);
	sp->s_inode_super = ip;
	sp->s_readonly = 0;
}

static void fsInit() {
/* Initialize global variables, tables, etc. */
	register FProc *fp;
	register Inode *ip;
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

	/* All process table entries have been set. Continue with FS initialization.
	 * Certain relations must hold for the file system to work at all. 
	 */
	if (OPEN_MAX > 127)
	  panic(__FILE__, "OPEN_MAX > 127", NO_NUM);
	if (NR_BUFS < 6)
	  panic(__FILE__, "NR_BUFS < 6", NO_NUM);
	if (INODE_SIZE != 64)
	  panic(__FILE__, "Inode size != 64", NO_NUM);
	if (OPEN_MAX > 8 * sizeof(long))
	  panic(__FILE__, "Too few bits in fpCloExec", NO_NUM);

	/* The following initializations are needed to let devOpcl succeed. */
	currFp = (FProc *) NULL;
	who = FS_PROC_NR;

	initBufPool();		/* Initialize buffer pool */
	buildDMap();		/* Build device table and map boot driver */
	loadRam();			/* Init RAM disk, load if it is root */
	loadSuper(rootDev);		/* Load super block for root device */

	/* The root device can now be accessed; set process directories. */
	for (fp = &fprocTable[0]; fp < &fprocTable[NR_PROCS]; ++fp) {
		if (fp->fp_pid != PID_FREE) {
			ip = getInode(rootDev, ROOT_INODE);
			dupInode(ip);
			fp->fp_root_dir = ip;
			fp->fp_work_dir = ip;
		}
	}
}

static void getWork() {
/* Normally wait for new input. However, if 'reviving' is nonzero, a
 * suspended process must be awakened.
 */
	register FProc *fp;

	if (reviving != 0) {
		/* Revive a suspended process. */
		for (fp = &fprocTable[0]; fp < &fprocTable[NR_PROCS]; ++fp) {
			if (fp->fp_revived == REVIVING) {
				who = (int) (fp - fprocTable);
				callNum = fp->fp_fd & BYTE;
				inMsg.fd = (fp->fp_fd >> 8) & BYTE;
				inMsg.buffer = fp->fp_buffer;
				inMsg.nbytes = fp->fp_nbytes;
				fp->fp_suspended = NOT_SUSPENDED;	/* No longer hanging */
				fp->fp_revived = NOT_REVIVING;
				--reviving;
				return;
			}
		}
		panic(__FILE__, "getWork couldn't revive anyone", NO_NUM);
	}

	/* Normal case. No one to revive. */
	if (receive(ANY, &inMsg) != OK)
	  panic(__FILE__, "FS receive error", NO_NUM);
	who = inMsg.m_source;
	callNum = inMsg.m_type;
}

void reply(int whom, int result) {
/* Send a reply to a user process. It may fail (if the process has just
 * been killed by a signal), so don't check the return code. If the send
 * fails, just ignore it.
 */
	int r;

	outMsg.reply_type = result;
	if ((r = send(whom, &outMsg)) != OK)
	  printf("FS: couldn't send reply %d: %d\n", result, r);
}

int main() {
/* This is the main program of the file system. The main loop consists of 
 * three major activities: getting new work, processing the work, and sending
 * the reply. This loop never terminates as long as the file system runs.
 */
	sigset_t sigset;
	int error;

	fsInit();

	while (true) {
		getWork();	/* Sets who and callNum */
		
		currFp = &fprocTable[who];
		superUser = currFp->fp_euid == SU_UID;	/* su? */
		
		/* Check for special control messages first. */
		if (callNum == SYS_SIG) {
			sigset = inMsg.NOTIFY_ARG;
			if (sigismember(&sigset, SIGKSTOP)) {
				doSync();
				sysExit(0);		/* Never returns */
			}
		} else if (callNum == SYN_ALARM) {
			/* Not a user request; system has expired one of our timers,
			 * currently only in use for select(). Check it.
			 */
			fsExpireTimers(inMsg.NOTIFY_TIMESTAMP);
		} else if (callNum & NOTIFY_MESSAGE) {
			/* Device notifies us of an event. */
			devStatus(&inMsg);
		} else {
			/* Call the internal function that does the work. */
			if (callNum < 0 || callNum >= NCALLS) {
				error = ENOSYS;
				printf("FS, warning illegal %d system call by %d\n", callNum, who);
			} else if (currFp->fp_pid == PID_FREE) {
				error = ENOSYS;
				printf("FS, bad process, who= %d, callNum = %d, slot1 = %d\n",
							who, callNum, inMsg.slot1);
			} else {
				error = (*callVec[callNum])();
			}

			/* Copy the results back to the user and send reply. */
			if (error != SUSPEND)
			  reply(who, error);
			if (readAheadInode != NIL_INODE)
			  readAhead();	/* Do block read ahead */
		}
	}
	return OK;		/* Shouldn't come here */
}
