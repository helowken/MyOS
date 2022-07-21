/* Table sizes */
#define NR_DIRECT_ZONES			7	/* # direct zone numbers in a inode */
#define NR_TOTAL_ZONES			10	/* total # zone numbers in a inode */

#define NR_FILPS				128	/* # slots in filp table */
#define NR_INODES				64	/* # slots in "in core" inode table */
#define NR_SUPERS				8	/* # slots in super block table */

/* File system types. */
#define SUPER_V3				0x4d5a	/* magic # for file system */

#define V3						3	/* Version number of V3 file system */

#define usizeof(t)				((unsigned) sizeof(t))

#define FS_BITMAP_CHUNKS(b)		((b) / usizeof(Bitchunk_t))
#define FS_BITCHUNK_BITS		(usizeof(Bitchunk_t) * CHAR_BIT)
#define FS_BITS_PER_BLOCK(b)	(FS_BITMAP_CHUNKS(b) * FS_BITCHUNK_BITS)

#define ZONE_NUM_SIZE			usizeof(Zone_t)		/* bytes in zone */
#define	INODE_SIZE				usizeof(DiskInode)	/* bytes in disk Inode */	
#define INDIRECT_ZONES(b)		((b) / ZONE_NUM_SIZE)	/* zones / indirect block */
#define INODES_PER_BLOCK(b)		((b) / INODE_SIZE)	/* disk inodes / blk */

#define DIR_ENTRY_SIZE			usizeof(DirEntry)	/* # bytes/dir entry */
#define NR_DIR_ENTRIES(b)		((b) / DIR_ENTRY_SIZE)	/* # dir entries/block */
#define SUPER_SIZE				usizeof(SuperBlock)	/* Superblock size */

#define ROOT_INODE				1		/* Inode number for root directory */
#define BOOT_BLOCK		((Block_t) 0)	/* Block number of boot block */
#define SUPER_BLOCK_BYTES		1024	/* Super block size */
#define SUPER_OFFSET_BYTES		1024	/* Super block bytes offset */
#define START_BLOCK				2		/* First block of FS (not counting super block) */

/* Zone index */
#define INDIR_ZONE_IDX			7		/* Single indirect zone index */
#define	DBL_IND_ZONE_IDX		(INDIR_ZONE_IDX + 1)	/* Double indirect zone index */

#define SU_UID			((uid_t) 0)		/* Super user's uid_t */
#define SYS_UID			((uid_t) 0)		/* uid_t for processes MM and INIT */
#define SYS_GID			((gid_t) 0)		/* gid_t for processes MM and INIT */
#define NORMAL					0		/* Forces getBlock() to do disk read */
#define NO_READ					1		/* Prevents getBlock() from doing disk read */
#define PREFETCH				2		/* Tells getBlock() not to read or mark dev */

#define CLEAN					0		/* Disk and memory copies identical */
#define DIRTY					1		/* Disk and memory copies differ */
#define ATIME					002		/* Set if atime field needs updating */
#define CTIME					004		/* Set if ctime field needs updating */
#define	MTIME					010		/* Set if mtime field needs updating */

#define END_OF_FILE			(-104)		/* EOF detected */
