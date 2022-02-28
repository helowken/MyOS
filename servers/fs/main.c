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
#include "file.h"
#include "fproc.h"
#include "param.h"

#define KB	1024

static void initBufPool() {
/* Initialize the buffer pool. */
	register Buf *bp;

	bufsInUse = 0;
	frontBP = &bufs[0];
	rearBP = &bufs[NR_BUFS - 1];

	for (bp = frontBP; bp <= rearBP; ++bp) {
		bp->b_block_num = NO_BLOCK;
		bp->b_dev = NO_DEV;
		bp->b_next = bp + 1;
		bp->b_prev = bp - 1;
		bp->b_hash_next = bp->b_next;
	}
	frontBP->b_prev = NIL_BUF;
	rearBP->b_next = NIL_BUF;
	rearBP->b_hash_next = NIL_BUF;
	bufHashTable[0] = frontBP;
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
	SuperBlock *imgSp;
	int imgBlockSize, ramBlockSize, factor;
	int s;
	block_t imgBlockNum, ramBlockNum;
	Buf *imgBP, *ramBP;
	
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
	} else if (outMsg.RESP_STATUS != OK) {
		/* Report and continue, unless RAM disk is required as ROOT FS. */
		if (rootDev != DEV_RAM) 
		  report("FS", "can't set RAM disk size", outMsg.RESP_STATUS);
		else
		  panic(__FILE__, "can't set RAM disk size", outMsg.RESP_STATUS);
	}

	/* See if we must load the RAM disk image, otherwise return. */
	if (rootDev != DEV_RAM)
	  return;

	/* Copy the blocks one at a time from the image to the RAM disk. */
	printf("Loading RAM disk onto /dev/ram:\33[23CLoaded:    0 KB");

	inodes[0].i_mode = I_BLOCK_SPECIAL;	
	inodes[0].i_size = LONG_MAX;
	inodes[0].i_dev = imageDev;
	inodes[0].i_zones[0] = imageDev;

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
		imgBP = readAhead(&inodes[0], imgBlockNum, 
				(off_t) imgBlockSize * imgBlockNum, imgBlockSize);
		factor = imgBlockSize / ramBlockSize;
		for (ramBlockNum = 0; ramBlockNum < factor; ++ramBlockNum) {
			ramBP = getBlock(rootDev, 
						imgBlockNum * factor + ramBlockNum, 
						NO_READ);
			memcpy(ramBP->b_data, 
					imgBP->b_data + ramBlockNum * ramBlockSize, 
					(size_t) ramBlockSize);
			ramBP->b_dirty = DIRTY;
			putBlock(ramBP, FULL_DATA_BLOCK);
		}
		putBlock(imgBP, FULL_DATA_BLOCK);
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
