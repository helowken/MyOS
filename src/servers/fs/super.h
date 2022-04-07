typedef struct {
	Ino_t s_inodes;			/* # usable inodes on the minor device */
	Zone_t s_zones;			/* number of zones */
	uint16_t s_imap_blocks;		/* # of blocks used by inode bit map */
	uint16_t s_zmap_blocks;		/* # of blocks used by zone bit map */
	Zone_t s_first_data_zone;	/* number of first data zone */
	uint16_t s_log_zone_size;	/* log2 of blocks/zone */
	Off_t s_max_size;		/* maximum file size on this device */
	uint16_t s_magic;		/* magic number to recognice super-blocks */

	uint16_t s_block_size;	/* block size in bytes. */
	char s_disk_version;	/* file system format sub-version */

	/* The following items are only used when the SuperBlock is in memory. */
	struct Inode *s_inode_super;	/* Inode for root dir of mounted file system */
	struct Inode *s_inode_mount;	/* Inode mounted on */
	uint32_t s_inodes_per_block;
	Dev_t s_dev;			/* Whose super block is this? */
	int s_readonly;			/* Set to 1 iff file sys mounted read only */
	int s_version;			/* File system version, zero means bad magic */
	int s_dzones;			/* # direct zones in an inode */
	int s_ind_zones;		/* # indirect zones per indirect block */
	Bit_t s_inode_search;	/* Inodes below this bit number are in use */
	Bit_t s_zone_search;	/* All zones below this bit number are in use */
} SuperBlock;

#define NIL_SUPER	((SuperBlock *) 0)
#define IMAP_OFFSET		2	/* Inode-map offset blocks */
