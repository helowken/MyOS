#include "sys/dir.h"
#include "dirent.h"

typedef struct Buf {
	/* Data portion of the buffer. */
	union {
		/* Ordinary user data */
		char b__data[MAX_BLOCK_SIZE];		
		/* Directory block */
		DirEntry b__dirs[NR_DIR_ENTRIES(MAX_BLOCK_SIZE)];
		/* Indirect block */
		Zone_t b__inds[INDIRECT_ZONES(MAX_BLOCK_SIZE)];
		/* Inode block */
		DiskInode b__inodes[INODES_PER_BLOCK(MAX_BLOCK_SIZE)];
		/* Bit map block */
		Bitchunk_t b__bitmaps[FS_BITMAP_CHUNKS(MAX_BLOCK_SIZE)];
	} b;

	/* Header portion of the buffer. */
	struct Buf *b_next;		/* Used to link all free bufs in a chain */
	struct Buf *b_prev;		/* Used to link all free bufs the other way */
	struct Buf *b_hash;		/* Used to link bufs on hash chains */
	Block_t b_block_num;	/* Block number of its (minor) device */
	Dev_t b_dev;		/* Major | minor device where block resides */
	char b_dirty;		/* CLEAN or DIRTY */
	char b_count;		/* Number of users of this buffer */
} Buf;
EXTERN Buf bufs[NR_BUFS];

#define NIL_BUF		((Buf *) 0)	/* Indicates absence of a buffer */

#define b_data		b.b__data
#define b_dirs		b.b__dirs
#define b_inds		b.b__inds
#define b_inodes	b.b__inodes
#define b_bitmaps	b.b__bitmaps

EXTERN Buf *bufHashTable[NR_BUF_HASH];		/* The buffer hash table */
EXTERN Buf *frontBuf;		/* Points to least recently used free block */
EXTERN Buf *rearBuf;		/* Points to most recently used free block */
EXTERN int bufsInUse;		/* # bufs currenly in use (not on free list) */

#define INODE_BLOCK			0	/* Inode block */
#define DIR_BLOCK			1	/* Directory block */
#define INDIRECT_BLOCK		2	/* Pointer block */
#define	MAP_BLOCK			3	/* Bit map */
#define FULL_DATA_BLOCK		5	/* Data, fully used */
#define	PARTIAL_DATA_BLOCK	6	/* Data, partly used */

