#ifndef _STAT_H
#define _STAT_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

struct stat {
	dev_t st_dev;		/* major/minor device number */
	ino_t st_ino;		/* i-node number */
	mode_t st_mode;		/* file mode, protection bits, etc. */
	nlink_t st_nlink;	/* links */
	uid_t st_uid;		/* the file's owner */
	gid_t st_gid;		/* the file's group */
	dev_t st_rdev;
	off_t st_size;		/* file size */
	time_t st_atime;	/* time of last access */
	time_t st_mtime;	/* time of last data modification */
	time_t st_ctime;	/* time of last file status change */
};

/* Traditional mask definitions for st_mode. */
#define S_IFMT		((mode_t) 0170000)	/* Type of file */
#define S_IFLNK		((mode_t) 0120000)	/* Symbolic link, not implemented */
#define S_IFREG		((mode_t) 0100000)	/* Regular */
#define S_IFBLK		0060000		/* Block special */
#define S_IFDIR		0040000		/* Directory */
#define S_IFCHR		0020000		/* Character special */
#define S_IFIFO		0010000		/* This is a FIFO */
#define S_ISUID		0004000		/* Set user id on execution */
#define S_ISGID		0002000		/* Set group id on execution */
#define	S_ISVTX		0001000		/* Save swapped text even after use */

/* POSIX masks for st_mode. */
#define S_IRWXU		00700		/* Owner:  rwx------ */
#define S_IRUSR		00400		/* Owner:  r-------- */
#define S_IWUSR		00200		/* Owner:  -w------- */
#define S_IXUSR		00100		/* Owner:  --x------ */

#define S_IRWXG		00070		/* Group:  ---rwx--- */
#define S_IRGRP		00040		/* Group:  ---r----- */
#define S_IWGRP		00020		/* Group:  ----w---- */
#define S_IXGRP		00010		/* Group:  -----x--- */

#define S_IRWXO		00007		/* Other:  ------rwx */
#define S_IROTH		00004		/* Other:  ------r-- */
#define S_IWOTH		00002		/* Other:  -------w- */
#define S_IXOTH		00001		/* Other:  --------x */

/* The following macros test st_mode. */
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)	/* Is a reg file */
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)	/* Is a directory */
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)	/* Is a char spec */
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)	/* Is a block spec */
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	/* Is a pipe/FIFO */
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)	/* Is a symbol link */

int stat(const char *path, struct stat *buf);

#endif
