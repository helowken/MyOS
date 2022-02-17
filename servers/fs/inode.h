typedef struct {
	Mode_t i_mode;		/* File type, protection, etc. */
	Nlink_t i_nlinks;	/* How many links to this file */
	uint16_t i_uid;		/* User id of the file's owner */
	uint16_t i_gid;		/* Group id */
	Off_t i_size;		/* Current file size in bytes */
	Time_t i_atime;		/* When was file data last accessed */
	Time_t i_mtime;		/* When was file data last changed */
	Time_t i_ctime;		/* When was inode data last changed */
	Zone_t i_zones[NR_TOTAL_ZONES];	/* Block nums for direct, indirect, and double indirect */

	/* The following items are not present on the disk. */
	Dev_t i_dev;		/* Which device is the inode on */
	Ino_t i_num;		/* Inode number on its (minor) device */
	int i_count;		/* # times inode used; 0 means slot is free */
	int i_direct_zones;	/* # direct zones (NR_DIRECT_ZONES) */
	int i_ind_zones;	/* # indirect zones per indirect block */
	SuperBlock *i_sup;	/* Pointer to super block for inode's device */
	char i_dirty;		/* CLEAN or DIRTY */
	char i_pipe;		/* Set to I_PIPE if pipe */
	char i_mount;		/* This bit is set if file mounted on */
	char i_seek;		/* Set on LSEEK, cleared on READ/WRITE */
	char i_update;		/* The ATIME, CTIME, and MTIME bits are here */
} Inode;

EXTERN Inode inode[NR_INODES];

#define NIL_INODE	((Inode *) 0)	/* Indicates absence of inode slot */
