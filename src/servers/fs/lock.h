typedef struct {
	short lock_type;	/* F_RDLCK or F_WRLCK; 0 means unused slot */
	pid_t lock_pid;		/* Pid of the process holding the lock */
	Inode *lock_inode;	/* Pointer to the inode locked. */
	off_t lock_first;	/* Offset of first byte locked */
	off_t lock_last;	/* Offset of last byte locked */
} FileLock;

#define NIL_FILELOCK	((FileLock *) 0)
