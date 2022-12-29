#include "stdlib.h"
#include "string.h"
#include "errno.h"

#define PtrInt		long
#define BRK_SIZE	4096
#define PTR_SIZE	((int) sizeof(void *))
#define align(x, a)	(((x) + (a - 1)) & ~(a - 1))
#define nextSlot(p)	(* (void **) ((p) - PTR_SIZE))
#define nextFree(p)	(* (void **) (p))

/* A short explanation of the data structure and algorithms.
 * An area returned by malloc() is called a slot. Each slot
 * contains the number of bytes requested, but preceeded by
 * an extra pointer to the next slot in memory.
 * 'bottom' and 'top' point to the first/last slot.
 * More memory is asked for using brk() and appended to top.
 * The list of free slots is maintained to keep malloc() fast.
 * 'empty' points the first free slot. Free slots are linked
 * together by a pointer at the start of the user visible 
 * part, so just after the next-slot pointer.
 * Free slots are merged together by free().
 */

extern void *sbrk(int);
extern int brk(void *);

static void *bottom, *top, *empty;

static int grow(size_t len) {
	register char *p;

	if ((char *) top + len < (char *) top ||
			(p = (char *) align((PtrInt) top + len, BRK_SIZE)) < (char *) top) {
		errno = ENOMEM;
		return 0;
	}
	if (brk(p) != 0)
	  return 0;
	nextSlot(top) = p;
	nextSlot(p) = 0;
	free(top);
	top = p;
	return 1;
}

void *malloc(size_t size) {
	register char *prev, *p, *next, *new;
	register unsigned len, numTries;

	if (size == 0)
	  return NULL;

	for (numTries = 0; numTries < 2; ++numTries) {
		if ((len = align(size, PTR_SIZE) + PTR_SIZE) < 2 * PTR_SIZE) {
			errno = ENOMEM;
			return NULL;
		}
		if (bottom == 0) {
			if ((p = sbrk(2 * PTR_SIZE)) == (char *) - 1)
			  return NULL;
			p = (char *) align((PtrInt) p, PTR_SIZE);
			p += PTR_SIZE;
			top = bottom = p;
			nextSlot(p) = 0;
		}
		for (prev = 0, p = empty; p != 0; prev = p, p = nextFree(p)) {
			next = nextSlot(p);
			new = p + len;		/* Easily overflows! */
			if (new > next || new <= p)
			  continue;			/* Too small */
			if (new + PTR_SIZE < next) {	/* Too big , so split */
				/* + PTR_SIZE avoids tiny slots on free list */
				nextSlot(new) = next;
				nextSlot(p) = new;
				nextFree(new) = nextFree(p);
				nextFree(p) = new;
			}
			if (prev)
			  nextFree(prev) = nextFree(p);
			else
			  empty = nextFree(p);
			return p;
		}
		if (grow(len) == 0)
		  break;
	}
	return NULL;
}

void free(void *ptr) {
	register char *prev, *next;
	char *p = ptr;

	if (p == 0)
	  return;

	for (prev = 0, next = empty; next != 0; prev = next, next = nextFree(next)) {
		if (p < next)
		  break;
	}
	nextFree(p) = next;
	if (prev)
	  nextFree(prev) = p;
	else
	  empty = p;

	if (next) {
		if (nextSlot(p) == next) {	/* Merge p and next */
			nextSlot(p) = nextSlot(next);
			nextFree(p) = nextFree(next);
		}
	} 
	if (prev) {
		if (nextSlot(prev) == p) {	/* Merge prev and p */
			nextSlot(prev) = nextSlot(p);
			nextFree(prev) = nextFree(p);
		}
	}
}

void *realloc(void *oldPtr, size_t size) {
	register char *prev, *p, *next, *new;
	char *old = oldPtr;
	register size_t len, n;

	if (old == 0)
	  return malloc(size);
	if (size == 0) {
		free(old);
		return NULL;
	}
	len = align(size, PTR_SIZE) + PTR_SIZE;
	next = nextSlot(old);
	n = (int) (next - old);		/* Old length */

	/* Extend old if there is any free space just behind it */
	for (prev = 0, p = empty; p != 0; prev = p, p = nextFree(p)) {
		if (p > next)
		  break;
		if (p == next) {	/* 'next' is a free slot: merge */
			nextSlot(old) = nextSlot(p);
			if (prev) 
			  nextFree(prev) = nextFree(p);
			else
			  empty = nextFree(p);
			break;
		}
	}
	new = old + len;

	/* Can we use the old, possibly extended slot? */
	if (new <= next && new >= old) {	/* It does fit */
		if (new + PTR_SIZE < next) {	/* Too big, so split */
			/* + PTR_SIZE avoids tiny slots on free list */
			nextSlot(new) = next;
			nextSlot(old) = new;
			free(new);
		}
		return old;
	}
	if ((new = malloc(size)) == NULL)	/* It didn't fit */
	  return NULL;
	memcpy(new, old, n);	/* n < size */
	free(old);
	return new;
}







