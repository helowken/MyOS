#include "sys/dir.h"
#include "dirent.h"

typedef struct Buf {

	/* Header portion of the buffer. */
	Buf *b_next;		/* Used to link all free bufs in a chain */
	Buf *b_prev;		/* Used to link all free bufs the other way */
	Buf *b_hash;		/* Used to link bufs on hash chains */
	block_t b_block_num;	/* Block number of its (minor) device */
	dev_t b_dev;		/* Major | minor device where block resides */
	char b_dirty;		/* CLEAN or DIRTY */
	char b_count;		/* Number of users of this buffer */
} Buf;
EXTERN Buf bufs[NR_BUFS];

#define NIL_BUF		((Buf *) 0)	/* Indicates absence of a buffer */

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

