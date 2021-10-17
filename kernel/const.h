#ifndef CONST_H
#define CONST_H

#include "ibm/interrupt.h"
#include "minix/config.h"
#include "config.h"

/* To translate an address in kernel space to a physical address. */
#define vir2Phys(vir)	(kernelInfo.dataBase + (vir_bytes) (vir))

/* Map a process number to a privilege structure id. */
#define s_nrToId(n)	(NR_TASKS + (n) + 1)

#endif
