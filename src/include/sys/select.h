#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

#include "sys/time.h"
#include "sys/types.h"
#include "limits.h"
#include "string.h"



/* Possible select() operation types; read, write, errors */
/* (FS/driver internal use only) */
#define SEL_RD		(1 << 0)
#define SEL_WR		(1 << 1)
#define SEL_ERR		(1 << 2)
#define SEL_NOTIFY	(1 << 3)	/* Not a real select operation */

#endif
