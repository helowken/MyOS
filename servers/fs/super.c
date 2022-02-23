#include "fs.h"
#include "string.h"
#include "minix/com.h"
#include "buf.h"
#include "super.h"
#include "inode.h"
#include "const.h"

int readSuper(SuperBlock *sp) {
/* Read a superblock. */
	dev_t dev;
	int magic, r;
	static char sbBuf[SUPER_BLOCK_BYTES];
	
	dev = sp->s_dev;	/* Save device (will be overwritten by copy) */
	if (dev == NO_DEV)
	  panic(__FILE__, "request for superblock of NO_DEV", NO_NUM);
	r = devIO(DEV_READ, dev, FS_PROC_NR, sbBuf, 
				SUPER_OFFSET_BYTES, SUPER_BLOCK_BYTES, 0);
	if (r != SUPER_BLOCK_BYTES)
	  return EINVAL;
	
	memcpy(sp, sbBuf, sizeof(*sp));
	sp->s_dev = NO_DEV;		/* Restore later */
	magic = sp->s_magic;	/* Determines file system type */
}
