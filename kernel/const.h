#ifndef CONST_H 
#define CONST_H

#include "limits.h"
#include "ibm/interrupt.h"
#include "ibm/ports.h"
#include "minix/config.h"
#include "config.h"

/* To translate an address in kernel space to a physical address. */
#define vir2Phys(vir)	(kernelInfo.dataBase + (vir_bytes) (vir))

/* Map a process number to a privilege structure id. */
#define s_nrToId(n)	(NR_TASKS + (n) + 1)

/* Constants and macros for bit map manipulation. */
#define BITCHUNK_BITS	(sizeof(bitchunk_t) * CHAR_BIT)
#define BITMAP_CHUNKS(nr_bits)	( ((nr_bits) + BITCHUNK_BITS - 1) / BITCHUNK_BITS )
#define MAP_CHUNK(map, bit)	(map)[ ((bit) / BITCHUNK_BITS) ]
#define CHUNK_OFFSET(bit)	((bit) % BITCHUNK_BITS)

#define getSysBit(map, bit)	( MAP_CHUNK(map.chunk, bit) & (1 << CHUNK_OFFSET(bit)) )
#define setSysBit(map, bit) ( MAP_CHUNK(map.chunk, bit) |= (1 << CHUNK_OFFSET(bit)) )
#define unsetSysBit(map, bit)	( MAP_CHUNK(map.chunk, bit) &= ~(1 << CHUNK_OFFSET(bit)) )
#define NR_SYS_CHUNKS	BITMAP_CHUNKS(NR_SYS_PROCS)

/* Program stack words and masks. */
#define INIT_PSW		0x0200	/* Initial psw */
#define INIT_TASK_PSW	0x1200	/* Initial psw for tasks (with IOPL = 1) */
#define TRACE_BIT		0x0100	/* OR this with psw in procTable[] for tracing */
#define SETPSW(rp, newState)	/* Permits only certain bits to be set: CF, PF, AF, ZF, SF, DF, OF */ \
	( (rp)->p_reg.psw = ((rp)->p_reg.psw & ~0xCD5) | ((newState) & 0xCD5) )

/* Disable / enable hardware interrupts. The parameters of lock() and unlock()
 * are used when debugging is enabled. See debug.h for more information.
 */
#define lock(c, v)	disableInterrupt();
#define unlock(c)	enableInterrupt();

#endif
