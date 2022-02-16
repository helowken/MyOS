/* Table sizes */
#define NR_DIRECT_ZONES			7	/* # direct zone numbers in a inode */
#define NR_TOTAL_ZONES			10	/* total # zone numbers in a inode */

/* File system types. */
#define SUPER_V3				0x4d5a	/* magic # for file system */

#define usizeof(t)				((unsigned) sizeof(t))

#define FS_BITMAP_CHUNKS(b)		((b) / usizeof(uint16_t))
#define FS_BITCHUNK_BITS		(usizeof(uint16_t) * CHAR_BIT)
#define FS_BITS_PER_BLOCK(b)	(FS_BITMAP_CHUNKS(b) * FS_BITCHUNK_BITS)

#define ZONE_NUM_SIZE			usizeof(zone_t)		/* bytes in zone */
#define	INODE_SIZE				usizeof(Inode)		/* bytes in disk Inode */	
#define INDIRECT_ZONES(b)		((b) / ZONE_NUM_SIZE)	/* zones / indirect block */
#define INODES_PER_BLOCK(b)		((b) / INODE_SIZE)	/* disk inodes / blk */

#define DIR_ENTRY_SIZE			usizeof(DirEntry)
#define NR_DIR_ENTRIES(b)		((b) / DIR_ENTRY_SIZE)
