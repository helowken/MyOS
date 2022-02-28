#include "fs.h"
#include "string.h"
#include "minix/com.h"
#include "const.h"

#define SECTOR_SIZE		512

int readSuper(SuperBlock *sp) {
/* Read a superblock. */
	dev_t dev;
	int magic, r;
	static char sbBuf[SUPER_BLOCK_BYTES];
	uint16_t bs;
	
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

	if (magic != SUPER_V3) 
	  return EINVAL;

	bs = sp->s_block_size;
	if (bs > MAX_BLOCK_SIZE) {
		printf("Filesystem block size is %d kB; maximum filesystem\n"
		   "block size is %d kB. This limit can be increased by recompiling.\n",
		   bs / 1024, MAX_BLOCK_SIZE / 1024);
		return EINVAL;
	}
	if (bs < MIN_BLOCK_SIZE || bs % SECTOR_SIZE != 0 ||
			SUPER_SIZE > bs || bs % INODE_SIZE != 0)
	  return EINVAL;
	sp->s_inodes_per_block = INODES_PER_BLOCK(bs);
	sp->s_dzones = NR_DIRECT_ZONES;
	sp->s_ind_zones = INDIRECT_ZONES(bs);
	sp->s_inode_search = 0;
	sp->s_zone_search = 0;
	sp->s_version = V3;

	/* Make a few basic checks to see if super block looks reasonable. */
	if (sp->s_imap_blocks < 1 || sp->s_zmap_blocks < 1 || sp->s_inodes < 1 || 
				sp->s_zones < 1 || (unsigned) sp->s_log_zone_size > 4) {
		printf("not enough inode map or zone map blocks, \n");
		printf("or not enough inodes, or not enough zones, "
			   "or zone size too large\n");
		return EINVAL;
	}
	sp->s_dev = dev;	/* Restore device number */
	return OK;
}

int getBlockSize(dev_t dev) {
/* Search the superblock table for this device. */
	register SuperBlock *sp;

	if (dev == NO_DEV)
	  panic(__FILE__, "request for block size of NO_DEV", NO_NUM);

	for (sp = &superBlocks[0]; sp < &superBlocks[NR_SUPERS]; ++sp) {
		if (sp->s_dev == dev) 
		  return sp->s_block_size;
	}

	/* No mounted filesystem? use this block size then. */
	return MIN_BLOCK_SIZE;
}
