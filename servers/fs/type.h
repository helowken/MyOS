typedef struct {
	Mode_t mode;		/* File type, protection, etc. */
	Nlink_t nlinks;		/* How many links to this file */
	uint16_t uid;		/* User id of the file's owner */
	uint16_t gid;		/* Group id */
	Off_t size;			/* Current file size in bytes */
	Time_t atime;		/* When was file data last accessed */
	Time_t mtime;		/* When was file data last changed */
	Time_t ctime;		/* When was inode data last changed */
	Zone_t zones[NR_TOTAL_ZONES];	/* Block nums for direct, indirect, and double indirect */
} DiskInode;
