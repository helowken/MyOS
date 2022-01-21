#define _POSIX_SOURCE		1
#define _MINIX				1
#define _SYSTEM				1

#include "minix/config.h"
#include "minix/type.h"
#include "minix/ipc.h"
#include "minix/com.h"
#include "minix/callnr.h"
#include "sys/types.h"
#include "minix/const.h"
#include "minix/syslib.h"
#include "minix/sysutil.h"

#include "string.h"
#include "limits.h"
#include "stddef.h"
#include "errno.h"

//#include "minix/partition.h"
//#include "minix/u64.h"


/* Parameters for the disk drive. */
#define SECTOR_SIZE		512		/* Physical sector size in bytes */
#define SECTOR_SHIFT	9		/* For division */
#define SECTOR_MASK		511		/* and remainder */
