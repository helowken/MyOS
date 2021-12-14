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
#define NIL_HOLE	(Hole *) 0		

static struct Hole {
	struct Hole *next;	/* Pointer to next entry on the list */
	phys_clicks base;	/* Where does the hole begin? */
	phys_clicks len;	/* How big is the hole? */
} holes[NR_HOLES];

typedef struct Hole Hole;

static Hole *holeHead;		/* Pointer to first hole */
static Hole *freeSlots;		/* Pointer to list of unused table slots */
static phys_clicks swapBase;		/* Memory offset chosen as swap base */
static phys_clicks swapMaxSize;		/* Maximum amount of swap "memory" possible */

static void deleteSlot(Hole *prevHp, Hole *hp) {
/* Remove an entry from the hole list. This procedure is called when a
 * request to allocate memory removes a hole in its entirety, thus reducing
 * the numbers of holes in memory, and requiring the elimination of one
 * entry in the hole list.
 */
	if (hp == holeHead) 
	  holeHead = hp->next;
	else
	  prevHp->next = hp->next;

	hp->next = freeSlots;
	freeSlots = hp;
}

static void merge(Hole *hp) {
/* Check for contiguous holes and merge any found. Contiguous holes can occur
 * when a block of memory is freed, and it happens to abut another hole on
 * either or both ends. The pointer 'hp' points to the first of a series of
 * three holes that can potentially all be merged together.
 */
	register Hole *nextHp;

	/* If 'hp' points to the last hole, no merging is possible. If it does not,
	 * try to absorb its successor into it and free the successor's table entry.
	 */
	if ((nextHp = hp->next) == NIL_HOLE)
	  return;
	if (hp->base + hp->len == nextHp->base) {
		hp->len += nextHp->len;		/* First one gets second one's memory */
		deleteSlot(hp, nextHp);
	} else {
		hp = nextHp;
	}

	/* If 'hp' now points to the last hole, return; otherwise, try to absorb its
	 * successor into it.
	 */
	if ((nextHp = hp->next) == NIL_HOLE)
	  return;
	if (hp->base + hp->len == nextHp->base) {
		hp->len += nextHp->len;		
		deleteSlot(hp, nextHp);
	}
}

void freeMemory(phys_clicks base, phys_clicks clicks) {
/* Return a block of free memory to the hole list. The parameters tell where
 * the block starts in physical memory and how big it is. The block is added
 * to the hole list. If it is contiguous with an existing hole on either end,
 * it is merged with the hole or holes.
 */
	register Hole *hp, *newHp, *prevHp;

	if (clicks == 0)
	  return;
	if ((newHp = freeSlots) == NIL_HOLE)
	  panic(__FILE__, "Hole table full", NO_NUM);
	newHp->base = base;
	newHp->len = clicks;
	freeSlots = newHp->next;
	hp = holeHead;

	/* If this block's address is numerically less than the lowest hole currently
	 * available, or if no holes are currently available, put this hole on the
	 * front of the hole list.
	 */
	if (hp == NIL_HOLE || base <= hp->base) {
		/* Block to be freed goes on front of the hole list. */
		newHp->next = hp;
		holeHead = newHp;
		merge(newHp);
		return;
	}

	/* Block to be returned does not go on front of hole list. */
	prevHp = NIL_HOLE;
	while (hp != NIL_HOLE && base > hp->base) {
		prevHp = hp;
		hp = hp->next;
	}

	/* We found where it goes. Insert block after 'prevHp'. */
	newHp->next = hp;
	prevHp->next = newHp;
	merge(prevHp);		/* Sequence is 'prevHp', 'newHp', 'hp' */
}

void initMemory(Memory *chunks, phys_clicks *freeClicks) {
/* Initialize hole lists. There are two lists: 'hole_head' points to a linked
 * list of all the holes (unused memory) in the system; 'free_slots' points to
 * a linked list of table entries that are not in use. Initially, the former
 * list has one entry for each chunk of physical memory, and the second
 * list links together the remaining table slots. As memory becomes more
 * fragmented in the course of time (i.e., the initial big holes break up into
 * smaller holes), new table slots are needed to represent them. These slots
 * are taken from the list headed by 'free_slots'.
 */
	int i;
	register Hole *hp;

	/* Put all holes on the free list. */
	for (hp = &holes[0]; hp < &holes[NR_HOLES]; ++hp) {
		hp->next = hp + 1;
	}
	holes[NR_HOLES - 1].next = NIL_HOLE;
	holeHead = NIL_HOLE;
	freeSlots = &holes[0];
	
	/* Use the chunks of physical memory to allocate holes. */
	*freeClicks = 0;
	for (i = NR_MEMS - 1; i >= 0; --i) {
		if (chunks[i].size > 0) {
			freeMemory(chunks[i].base, chunks[i].size);
			*freeClicks += chunks[i].size;
			if (swapBase < chunks[i].base + chunks[i].size)
			  swapBase = chunks[i].base + chunks[i].size;
		}
	}

	/* The swap area is represented as a hole above and separate of regular
	 * memory. A hole at the size of the swap file is allocated on "swapon".
	 */
	++swapBase;		/* Make separate */
	swapMaxSize = 0 - swapBase;		/* Maximum we can possibly use */
}


