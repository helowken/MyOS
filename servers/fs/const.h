/* Table sizes */
#define DR_DIRECT_ZONES		7	/* # direct zone numbers in a inode */
#define NR_TOTAL_ZONES		10	/* total # zone numbers in a inode */

#define usizeof(t)			((unsigned) sizeof(t))

#define	INODE_SIZE			usizeof (INode)		/* bytes in disk INode */	
#define INODES_PER_BLOCK(b)	((b) / INODE_SIZE)	/* disk inodes / blk */
