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
	struct Hole *h_next;	/* Pointer to next entry on the list */
	phys_clicks h_base;	/* Where does the hole begin? */
	phys_clicks h_len;	/* How big is the hole? */
} holes[NR_HOLES];

typedef struct Hole Hole;

static Hole *holeHead;		/* Pointer to first hole */
static Hole *freeSlots;		/* Pointer to list of unused table slots */
static int swapFd = -1;		/* File descriptor of open swap file/device */
static phys_clicks swapBase;		/* Memory offset chosen as swap base */
static phys_clicks swapMaxSize;		/* Maximum amount of swap "memory" possible */
static MProc *inQueue;		/* Queue of processes wanting to swap in */
//static MProc *outSwap = &mprocTable[0];		/* Outswap candidate? */

static void deleteSlot(Hole *prevHp, Hole *hp) {
/* Remove an entry from the hole list. This procedure is called when a
 * request to allocate memory removes a hole in its entirety, thus reducing
 * the numbers of holes in memory, and requiring the elimination of one
 * entry in the hole list.
 */
	if (hp == holeHead) 
	  holeHead = hp->h_next;
	else
	  prevHp->h_next = hp->h_next;

	hp->h_next = freeSlots;
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
	if ((nextHp = hp->h_next) == NIL_HOLE)
	  return;
	if (hp->h_base + hp->h_len == nextHp->h_base) {
		hp->h_len += nextHp->h_len;		/* First one gets second one's memory */
		deleteSlot(hp, nextHp);
	} else {
		hp = nextHp;
	}

	/* If 'hp' now points to the last hole, return; otherwise, try to absorb its
	 * successor into it.
	 */
	if ((nextHp = hp->h_next) == NIL_HOLE)
	  return;
	if (hp->h_base + hp->h_len == nextHp->h_base) {
		hp->h_len += nextHp->h_len;		
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
	  panic(__FILE__, "hole table full", NO_NUM);
	newHp->h_base = base;
	newHp->h_len = clicks;
	freeSlots = newHp->h_next;
	hp = holeHead;

	/* If this block's address is numerically less than the lowest hole currently
	 * available, or if no holes are currently available, put this hole on the
	 * front of the hole list.
	 */
	if (hp == NIL_HOLE || base <= hp->h_base) {
		/* Block to be freed goes on front of the hole list. */
		newHp->h_next = hp;
		holeHead = newHp;
		merge(newHp);
		return;
	}

	/* Block to be returned does not go on front of hole list. */
	prevHp = NIL_HOLE;
	while (hp != NIL_HOLE && base > hp->h_base) {
		prevHp = hp;
		hp = hp->h_next;
	}

	/* We found where it goes. Insert block after 'prevHp'. */
	newHp->h_next = hp;
	prevHp->h_next = newHp;
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
		hp->h_next = hp + 1;
	}
	holes[NR_HOLES - 1].h_next = NIL_HOLE;
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

void swapInQueue(register MProc *rmp) {
/* Put a swapped out process on the queue of processes to be swapped in. This
 * happens when such a process gets a signal, or if a reply message must be
 * sent, like when a process doing a wait() has a child that exits.
 */
	MProc **xp;

	if (rmp->mp_flags & SWAPIN) 
	  return;

	for (xp = &inQueue; *xp != NULL; xp = &(*xp)->mp_swap_in_q) {
	}
	*xp = rmp;
	rmp->mp_swap_in_q = NULL;
	rmp->mp_flags |= SWAPIN;
}

void swapIn() {
/* Try to swap in a process on the inswap queue. We want to send it a message,
 * interrupt it, or something.
 */	
	//TODO
}

static bool swapOut() {
/* Try to find a process that can be swapped out. Candidates are those blocked
 * on a system call that PM handles, like wait(), pause() or sigsuspend().
 */
	//TODO
	if (swapFd == -1) return false; //TODO
	return false;
}

phys_clicks allocMemory(phys_clicks clicks) {
/* Allocate a block of memory from the free list using first fit. The block
 * consists of a sequence of contiguous bytes, whose length in clicks is 
 * given by 'clicks'. A pointer to the block is returned. The block is
 * always on a click boundary. This procedure is called when memory is
 * needed for FORK or EXEC. Swap other processes out if needed.
 */
	register Hole *hp, *prevHp;
	phys_clicks oldBase;
	
	do {
		prevHp = NIL_HOLE;
		hp = holeHead;
		while (hp != NIL_HOLE && hp->h_base < swapBase) {
			if (hp->h_len >= clicks) {
				/* We found a hole that is big enough. Use it. */
				oldBase = hp->h_base;	/* Remember where it started */
				hp->h_base += clicks;	/* Bite a piece off */
				hp->h_len -= clicks;	/* Ditto */

				/* Delete the hole if used up completely. */
				if (hp->h_len == 0)
				  deleteSlot(prevHp, hp);
				
				/* Return the start address of the acquired block. */
				return oldBase;
			}
			prevHp = hp;
			hp = hp->h_next;
		}
	} while (swapOut());	/* Try to swap some other process out */

	return NO_MEM;
}
