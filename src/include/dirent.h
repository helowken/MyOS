#ifndef _DIRENT_H
#define _DIRENT_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

#include "sys/dir.h"

/* The block size must be at least 1024 bytes, because otherwise
 * the superblock (at 1024 bytes) overlaps with other filesystem data.
 */
#define MIN_BLOCK_SIZE		1024

#define MAX_BLOCK_SIZE		4096

#endif
