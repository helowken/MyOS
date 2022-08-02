#include "fs.h"
#include "sys/stat.h"
#include "string.h"
#include "minix/com.h"
#include "minix/callnr.h"
#include "param.h"


void truncate(Inode *ip) {
/* Remove all the zones from the inode 'ip' and mark it dirty. */
	register block_t b;
	int fileType, scale, numIndZones, single, i;
	dev_t dev;
	off_t position;
	zone_t z, zoneSize, z1;
	bool wasPipe;
	Buf *bp;

	fileType = ip->i_mode & I_TYPE;		/* Check to see if file is special */
	if (fileType == I_CHAR_SPECIAL || fileType == I_BLOCK_SPECIAL)
	  return;
	dev = ip->i_dev;		/* Device on which inode resides. */
	scale = ip->i_sp->s_log_zone_size;
	zoneSize = (zone_t) ip->i_sp->s_block_size << scale;
	numIndZones = ip->i_ind_zones;

	/* Pipe can shrink, so adjust size to make sure all zones are removed. */
	wasPipe = (ip->i_pipe == I_PIPE);	/* True is this was a pipe */
	if (wasPipe)
	  ip->i_size = PIPE_SIZE(ip->i_sp->s_block_size);

	/* Step through the file a zone at a time, finding and freeing the zones. */
	for (position = 0; position < ip->i_size; position += zoneSize) {
		if ((b = readMap(ip, position)) != NO_BLOCK) {
			z = (zone_t) b >> scale;
			freeZone(dev, z);
		}
	}

	/* All the data zones have bee freed. Now free the indirect zones. */
	ip->i_dirty = DIRTY;
	if (wasPipe) {
		wipeInode(ip);		/* Clear out inode for pipes. */
		return;			/* Indirect slots contain file positions. */
	}
	single = ip->i_dir_zones;
	freeZone(dev, ip->i_zone[single]);	/* Single indirect zone */
	if ((z = ip->i_zone[single + 1]) != NO_ZONE) {
		/* Free all the single indirect zones pointed to by the double. */
		b = (block_t) z << scale;
		bp = getBlock(dev, b, NORMAL);	/* Get double indirect zone */
		for (i = 0; i < numIndZones; ++i) {
			z1 = readIndirZone(bp, i);
			freeZone(dev, z1);
		}

		/* Now free the double indirect zone itself. */
		putBlock(bp, INDIRECT_BLOCK);
		freeZone(dev, z);
	}
}
