#ifndef _SYS_DIR_H
#define _SYS_DIR_H

#include <sys/types.h>

#define DIR_BLOCK_SIZE	512		/* size of directory block */

#ifndef DIRSIZ 
#define DIRSIZ	60
#endif

typedef struct {
	ino_t d_ino;
	char d_name[DIRSIZ];
} DirEntry;

#endif
