#ifndef CONST_H 
#define CONST_H

#include "limits.h"
#include "ibm/interrupt.h"
#include "minix/config.h"
#include "config.h"

/* To translate an address in kernel space to a physical address. */
#define vir2Phys(vir)	(kernelInfo.dataBase + (vir_bytes) (vir))

/* Map a process number to a privilege structure id. */
#define s_nrToId(n)	(NR_TASKS + (n) + 1)

/* Constants and macros for bit map manipulation. */
#define BITCHUNK_BITS	(sizeof(bitchunk_t) * CHAR_BIT)
#define BITMAP_CHUNKS(nr_bits)	(((nr_bits) + BITCHUNK_BITS - 1) / BITCHUNK_BITS)

#endif
