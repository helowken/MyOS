#define _POSIX_SOURCE	1
#define _MINIX			1
#define _SYSTEM			1

#include <stdint.h>
#include <minix/config.h>
#include <sys/types.h>
#include <minix/const.h>
#include <minix/type.h>
#include <minix/dmap.h>

#include <limits.h>
#include <errno.h>

#include <minix/syslib.h>
#include <minix/sysutil.h>

#include "const.h"
#include "type.h"
#include "proto.h"
#include "fproc.h"
#include "glo.h"
