#define _POSIX_SOURCE		1
#define _MINIX				1
#define _SYSTEM				1

#include "minix/config.h"
#include "minix/type.h"
#include "minix/com.h"
#include "minix/callnr.h"
#include "sys/types.h"
#include "minix/const.h"
#include "minix/devio.h"
#include "minix/syslib.h"
#include "minix/sysutil.h"

#include "ibm/interrupt.h"
#include "ibm/bios.h"
#include "ibm/ports.h"

#include "string.h"
#include "signal.h"
#include "stdlib.h"
#include "limits.h"
#include "stddef.h"
#include "errno.h"
#include "unistd.h"
