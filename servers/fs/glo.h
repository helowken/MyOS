#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

/* File System global variables */
EXTERN struct FProc *fp;	/* Pointer to caller's FProc */
EXTERN int superUser;		/* 1 if caller is superUser, else 0 */
EXTERN dev_t rootDev;		/* Device number of the root device */

/* The parameters of the call are kept here. */
EXTERN Message inMsg;		/* The input message itself */
EXTERN Message outMsg;		/* The output message used for reply */
EXTERN int who;				/* Caller's proc number */
EXTERN int callNum;			/* System call number */

/* The following variables are used for returning results to the caller. */
EXTERN int errCode;			/* Temporary storage for error number */
EXTERN int rdwtErr;			/* Status of last disk I/O request */


EXTERN SuperBlock superBlocks[NR_SUPERS];
EXTERN Inode inodes[NR_INODES];


EXTERN Buf bufs[NR_BUFS];
EXTERN Buf *bufHashTable[NR_BUF_HASH];		/* The buffer hash table */
EXTERN Buf *frontBP;		/* Points to least recently used free block */
EXTERN Buf *rearBP;		/* Points to most recently used free block */
EXTERN int bufsInUse;		/* # bufs currenly in use (not on free list) */
