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


int stat(const char *path, struct stat *buf);

#endif
