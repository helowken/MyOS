typedef struct {
	ino_t s_inodes;			/* # usable inodes on the minor device */
	zone_t s_zones;			/* number of zones */
	short s_imap_blocks;		/* # of blocks used by inode bit map */
	short s_zmap_blocks;		/* # of blocks used by zone bit map */
	uint16_t s_first_data_zone;	/* number of first data zone */
	short s_log_zone_size;		/* log2 of blocks/zone */
	uint32_t s_max_size;		/* maximum file size on this device */
	short s_magic;				/* magic number to recognice super-blocks */

	uint16_t s_block_size;		/* block size in bytes. */
	char s_disk_version;		/* file system format sub-version */

	/* The following items are only used when the SuperBlock is in memory. */
} SuperBlock;

