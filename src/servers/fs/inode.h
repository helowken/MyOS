typedef struct Inode {
	Mode_t i_mode;		/* File type, protection, etc. */
	Nlink_t i_nlinks;	/* How many links to this file */
	uint16_t i_uid;		/* User id of the file's owner */
	uint16_t i_gid;		/* Group id */
	Off_t i_size;		/* Current file size in bytes */
	Time_t i_atime;		/* When was file data last accessed */
	Time_t i_mtime;		/* When was file data last changed */
	Time_t i_ctime;		/* When was inode data last changed */
	Zone_t i_zone[NR_TOTAL_ZONES];	/* Zone numbers for direct, indirect, and double indirect */

	/* The following items are not present on the disk. */
	Dev_t i_dev;		/* Which device is the inode on */
	Ino_t i_num;		/* Inode number on its (minor) device */
	int i_count;		/* # times inode used; 0 means slot is free */
	int i_dir_zones;	/* # direct zones (NR_DIRECT_ZONES) */
	int i_ind_zones;	/* # indirect zones per indirect block */
	SuperBlock *i_sp;	/* Pointer to super block for inode's device */
	char i_dirty;		/* CLEAN or DIRTY */
	char i_pipe;		/* Set to I_PIPE if pipe */
	char i_mount;		/* This bit is set if file mounted on */
	char i_seek;		/* Set on LSEEK, cleared on READ/WRITE */
	char i_update;		/* The ATIME, CTIME, and MTIME bits are here */
} Inode;

#define NIL_INODE	((Inode *) 0)	/* Indicates absence of inode slot */

/* Field values. Note that CLEAN and DIRTY are defined in "const.h" */
#define NO_PIPE		0	/* i_pipe is NO_PIPE if inode is not a pipe */
#define I_PIPE		1	/* i_pipe is I_PIPE if inode is a pipe */
#define NO_MOUNT	0	/* i_mount is NO_MOUNT if file not mounted on */
#define I_MOUNT		1	/* i_mount is I_MOUNT if file mounted on */
#define NO_SEEK		0	/* i_seek is NO_SEEK if last op was not SEEK */
#define I_SEEK		1	/* i_seek is I_SEEK if last op was SEEK */


