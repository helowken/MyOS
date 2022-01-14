typedef struct {
	uint16_t mode;		/* File type, protection, etc. */
	uint16_t nlinks;	/* How many links to this file */
	uint16_t uid;		/* User id of the file's owner */
	uint16_t gid;		/* Group id */
	uint32_t size;		/* Current file size in bytes */
	int32_t atime;		/* When was file data last accessed */
	int32_t mtime;		/* When was file data last changed */
	int32_t ctime;		/* When was inode data last changed */
	uint32_t zones[NR_TOTAL_ZONES];	/* Block nums for direct, indirect, and double indirect */
} INode;
