typedef struct {
	Mode_t d_mode;		/* File type, protection, etc. */
	Nlink_t d_nlinks;		/* How many links to this file */
	uint16_t d_uid;		/* User id of the file's owner */
	uint16_t d_gid;		/* Group id */
	Off_t d_size;			/* Current file size in bytes */
	Time_t d_atime;		/* When was file data last accessed */
	Time_t d_mtime;		/* When was file data last changed */
	Time_t d_ctime;		/* When was inode data last changed */
	Zone_t d_zone[NR_TOTAL_ZONES];	/* Zone numbers for direct, indirect, and double indirect */
} DiskInode;
