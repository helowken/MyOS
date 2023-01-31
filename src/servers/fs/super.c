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
	sp->s_dir_zones = NR_DIRECT_ZONES;
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

SuperBlock *getSuper(dev_t dev) {
/* Search the superblock table for this device. It is supposed to be there. */
	register SuperBlock *sp;

	if (dev == NO_DEV) 
	  panic(__FILE__, "request for superblock of NO_DEV", NO_NUM);

	for (sp = &superBlocks[0]; sp < &superBlocks[NR_SUPERS]; ++sp) {
		if (sp->s_dev == dev)
		  return sp;
	}

	/* Search failed. Something wrong. */
	panic(__FILE__, "can't find superblock for device (in decimal)", dev);
	return NIL_SUPER;
}

void freeBit(register SuperBlock *sp, int map, bit_t bitReturned) {
/* Return a zone or inode by turning off its bitmap bit. */
	unsigned block, word, bit;
	block_t startBlock;
	bitchunk_t k, mask;
	Buf *bp;

	if (sp->s_readonly)
	  panic(__FILE__, "can't free bit on read-only filesystem.", NO_NUM);

	if (map == IMAP) 
	  startBlock = START_BLOCK;	
	else 
	  startBlock = START_BLOCK + sp->s_imap_blocks;

	block = bitReturned / FS_BITS_PER_BLOCK(sp->s_block_size);
	word = (bitReturned % FS_BITS_PER_BLOCK(sp->s_block_size)) / FS_BITCHUNK_BITS;
	bit = bitReturned % FS_BITCHUNK_BITS;
	mask = 1 << bit;

	bp = getBlock(sp->s_dev, startBlock + block, NORMAL);
	k = bp->b_bitmaps[word];
	if (!(k & mask)) 
	  panic(__FILE__, map == IMAP ? "tried to free unused inode" :
				  "tried to free unused block", NO_NUM);

	k &= ~mask;
	bp->b_bitmaps[word] = k;
	bp->b_dirty = DIRTY;
	
	putBlock(bp, MAP_BLOCK);	/* TODO WRITE_IMMED? */
}

bit_t allocBit(SuperBlock *sp, int map, bit_t origin) {
/* Allocate a bit from a bit map and return its bit number. */

	block_t startBlock;		/* First bit block */
	bit_t mapBits;		/* How many bits are there in the bit map? */
	unsigned bitBlocks;		/* How many blocks are there in the bit map? */
	unsigned block, word, bCount;
	Buf *bp;
	bitchunk_t *wp, *wLimit, k;
	bit_t i, b;

	if (sp->s_readonly)
	  panic(__FILE__, "can't allocate bit on read-only filesystem.", NO_NUM);

	if (map == IMAP) {
		startBlock = START_BLOCK;
		mapBits = sp->s_inodes + 1;
		bitBlocks = sp->s_imap_blocks;
	} else {
		startBlock = START_BLOCK + sp->s_imap_blocks;
		mapBits = sp->s_zones - (sp->s_first_data_zone - 1);
		bitBlocks = sp->s_zmap_blocks;
	}

	/* Figure out where to start the bit search (depends on 'origin'). */
	if (origin >= mapBits)
	  origin = 0;

	/* Locate the starting place. */
	block = origin / FS_BITS_PER_BLOCK(sp->s_block_size);
	word = (origin % FS_BITS_PER_BLOCK(sp->s_block_size)) / FS_BITCHUNK_BITS;

	/* Iterate over all blocks plus one, because we start in the middle. 
	 * E.g, block = 3, bitBlocks = 6, word = n (n > 0),
	 * searching will start from 3[n] to 6, then wrap around, from 0 to 2, 
	 * and there is 3[0] ~ 3[n - 1] left, so need to plus 1.
	 */
	bCount = bitBlocks + 1;
	do {
		bp = getBlock(sp->s_dev, startBlock + block, NORMAL);		
		wLimit = &bp->b_bitmaps[FS_BITMAP_CHUNKS(sp->s_block_size)];
		
		/* Iterate over the words in block. */
		for (wp = &bp->b_bitmaps[word]; wp < wLimit; ++wp) {
			/* Does this word contain a free bit? */
			if (*wp == (bitchunk_t) ~0)
			  continue;

			/* Find and allocate the free bit. */
			k = *wp;
			for (i = 0; (k & (1 << i)) != 0; ++i) {
			}

			/* Bit number from the start of the bit map. */
			b = ((bit_t) block * FS_BITS_PER_BLOCK(sp->s_block_size)) +
				(wp - &bp->b_bitmaps[0]) * FS_BITCHUNK_BITS + i;
			
			/* Don't allocate bits beyond the end of the map, since there
			 * may be more bits left, but no more space for inode/zone.
			 */
			if (b >= mapBits)
			  break;

			/* Allocate and return bit number. */
			k |= 1 << i;
			*wp = k;
			bp->b_dirty = DIRTY;
			putBlock(bp, MAP_BLOCK);
			return b;
		}
		putBlock(bp, MAP_BLOCK);
		if (++block >= bitBlocks)
		  block = 0;	/* Last block, wrap around. */
		word = 0;		/* Start from 0 instead of middle */
	} while (--bCount > 0);
	
	return NO_BIT;		/* No bit could be allocated */
}

int mounted(Inode *ip) {
/* Report on whether the given inode is on a mounted (or ROOT) file system. */
	register SuperBlock *sp;
	register dev_t dev;

	dev = (dev_t) ip->i_zone[0];
	if (dev == rootDev)
	  return TRUE;	/* Inode is on root file system. */

	for (sp = &superBlocks[0]; sp < &superBlocks[NR_SUPERS]; ++sp) {
		if (sp->s_dev == dev)
		  return TRUE;
	}
	return FALSE;
}











