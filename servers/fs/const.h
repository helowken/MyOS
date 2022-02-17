/* Table sizes */
#define NR_DIRECT_ZONES			7	/* # direct zone numbers in a inode */
#define NR_TOTAL_ZONES			10	/* total # zone numbers in a inode */

#define NR_INODES				64	/* # slots in "in core" inode table */
#define NR_SUPERS				8	/* # slots in super block table */

/* File system types. */
#define SUPER_V3				0x4d5a	/* magic # for file system */

#define usizeof(t)				((unsigned) sizeof(t))

#define FS_BITMAP_CHUNKS(b)		((b) / usizeof(Bitchunk_t))
#define FS_BITCHUNK_BITS		(usizeof(Bitchunk_t) * CHAR_BIT)
#define FS_BITS_PER_BLOCK(b)	(FS_BITMAP_CHUNKS(b) * FS_BITCHUNK_BITS)

#define ZONE_NUM_SIZE			usizeof(Zone_t)		/* bytes in zone */
#define	INODE_SIZE				usizeof(DiskInode)		/* bytes in disk Inode */	
#define INDIRECT_ZONES(b)		((b) / ZONE_NUM_SIZE)	/* zones / indirect block */
#define INODES_PER_BLOCK(b)		((b) / INODE_SIZE)	/* disk inodes / blk */

#define DIR_ENTRY_SIZE			usizeof(DirEntry)
#define NR_DIR_ENTRIES(b)		((b) / DIR_ENTRY_SIZE)

#define ROOT_INODE				1		/* Inode number for root directory */
#define BOOT_BLOCK		((Block_t) 0)	/* Block number of boot block */
#define SUPER_BLOCK_BYTES		1024	/* Super block size */
#define SUPER_OFFSET_BYTES		1024	/* Super block bytes offset */
#define START_BLOCK				2		/* First block of FS (not counting super block) */
