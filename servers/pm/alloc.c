#include "pm.h"
#include "minix/com.h"
#include "minix/callnr.h"
#include "signal.h"
#include "stdlib.h"
#include "mproc.h"
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"

#define NR_HOLES	(2 * NR_PROCS)	/* Max # entries in hole table */
#define NIL_HOLE	(struct Hole *) 0		

static struct Hole {
	struct Hole *next;	/* Pointer to next entry on the list */
	phys_clicks base;	/* Where does the hole begin? */
	phys_clicks len;	/* How big is the hole? */
} holes[NR_HOLES];

void initMemory(Memory *chunks, phys_clicks *free) {
/* Initialize hole lists. There are two lists: */
}
