#define _SYSTEM			1	/* Get OK and negative error codes */
#define _MINIX			1	/* Tell headers to include MNIX stuff */
#define _POSIX_SOURCE	1

#include "sys/types.h"
#include "limits.h"
#include "errno.h"
#include "signal.h"

#include "minix/callnr.h"
#include "minix/config.h"
#include "minix/type.h"
#include "minix/const.h"
#include "minix/com.h"
#include "minix/syslib.h"
#include "minix/sysutil.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

#include "proto.h"
