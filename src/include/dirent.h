/* Note: The V7 format directory entries used under Minix must be transformed
 * into a struct direct with a d_name of at least 15 characters. Given that
 * we have to transform V7 entries anyhow it is little trouble to let the
 * routines understand the so-called "flex" directory format too.
 */

#ifndef _DIRENT_H
#define _DIRENT_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

#include "sys/dir.h"

/* FlexDir is a flexible directory entry. Actually it's a union of 8
 * characters and the 3 fields defined below.
 */

/* Flexible directory entry: */
typedef struct {	/* First slot in an entry */
	ino_t d_ino;
	unsigned char d_extent;
	char d_name[3];		/* Two characters for the shortest name */
} FlexDir;

/* Name of length len needs _EXTENT(len) extra slots. 
 *  5 = sizeof(d_ino) + sizeof(d_extent)
 */
#define _EXTENT(len)		(((len) + 5) >> 3)

/* Version 7 directory entry: */
typedef struct {
	ino_t d_ino;
	char d_name[DIRSIZ];
} V7Dir;

/* The block size must be at least 1024 bytes, because otherwise
 * the superblock (at 1024 bytes) overlaps with other filesystem data.
 */
#define MIN_BLOCK_SIZE		1024

/* The below is allocated in some parts of the system as the largest
 * a filesystem block can be. For instance, the boot monitor allocates
 * 3 of these blocks and has to fit within 64KB, so this can't be
 * increased without taking that into account.
 */
#define MAX_BLOCK_SIZE		4096

#define FLEX_PER_V7		(_EXTENT(DIRSIZ) + 1)
#define FLEX_PER_BLOCK	(MAX_BLOCK_SIZE / sizeof(FlexDir))

/* Definitions for the directory(3) routines: */
typedef struct {
	char _fd;		/* File descriptor of open directory */
	char _v7;		/* Directory is Version 7 */
	short _count;	/* How many objets in buf */
	off_t _pos;		/* Position in directory file */
	FlexDir *_ptr;	/* Next slot in buf */
	FlexDir _buf[FLEX_PER_BLOCK];	/* One block of a directory file */
	FlexDir _v7f[FLEX_PER_V7];		/* V7 entry transformed to flex */
} DIR;

#define _DIRENT_NAME_LEN	61

struct dirent {		/* Largest entry (8 slots) */
	ino_t d_ino;	/* I-node number */
	unsigned char d_extent;			/* Extended with this many slots */
	char d_name[_DIRENT_NAME_LEN];	/* Null terminated name */
};

/* Function Prototypes. */
int closedir(DIR *dirp);
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
void rewinddir(DIR *dirp);

#ifdef _MINIX
int seekdir(DIR *dirp, off_t pos);
off_t telldir(DIR *dirp);
#endif

#endif
