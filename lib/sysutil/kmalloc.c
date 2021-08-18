#include "code.h"
#include "debug.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"
#include "assert.h"

extern void *sbrk(size_t size);
extern int printf(char *fmt, ...);

typedef struct cell {
	size_t		size;
#if DEBUG
	unsigned	magic;
#endif
	struct	cell *next;
} cell_t;

#if UNIT_MAX <= 0xFFFF
#define MAGIC	0x537B
#else
#define MAGIC	0x537BC0D8
#endif

#define HDR_SIZE	offsetof(cell_t, next)

#define offset(cp, size)	((cell_t *) ((char *) (cp) + (size)))

#define cell2obj(cp)		((void *) ((char *) (cp) + HDR_SIZE))
#define obj2cell(op)		((cell_t *) ((char *) (op) - HDR_SIZE))

static cell_t *freeList;

void *malloc(size_t size) {
	cell_t **pcp, *cp, *next;

	size += HDR_SIZE;
	if (size < sizeof(cell_t))
	  size = sizeof(cell_t);

	// Align
	size = (size + sizeof(int) - 1) & ~(sizeof(int) - 1);

	// Space for a magic number at the end of the chunk.
	debug(size += sizeof(unsigned));

	for (;;) {
		// Do a first fit search.
		pcp = &freeList;
		while ((cp = *pcp) != NULL) {
			next = cp->next;
			assert(cp->magic == MAGIC);
			
			if (offset(cp, cp->size) == next) {
				// Join ajacent free cells.
				assert(next->magic == MAGIC);
				cp->size += next->size;
				cp->next = next->next;
				continue;
			}
			// Big enough?
			if (cp->size >= size)
			  break;
			pcp = &cp->next;
		}

		if (cp != NULL)
		  break;

		// Allocate a new chunk at the break.
		if ((cp = (cell_t *) sbrk(size)) == (cell_t *) -1)
		  return NULL;

		cp->size = size;
		cp->next = NULL;
		debug(cp->magic = MAGIC);
		// add to tail, then try to merge this new chunk with adjacent cells at next round.
		*pcp = cp;
	}

	// if it is big enough, break it up.
	if (cp->size >= size + sizeof(cell_t)) {
		next = offset(cp, cp->size);
		next->size = cp->size - size;
		next->next = cp->next;
		debug(next->magic = MAGIC);
		cp->size = size;
		cp->next = next;
	}

	*pcp = cp->next;
	debug(memset(cell2obj(cp), 0xAA, cp->size - HDR_SIZE));
	// Set magic at the end of the chunk.
	debug(((unsigned *) offset(cp, cp->size))[-1] = MAGIC);

	return cell2obj(cp);
}

void free(void *op) {
	cell_t **prev, *next, *cp;

	if (op == NULL)
	  return;

	cp = obj2cell(op);
	assert(cp->magic == MAGIC);
	assert(((unsigned *) offset(cp, cp->size))[-1] == MAGIC);

	// order by address so that merge can be done the next time.
	prev = &freeList;
	while ((next = *prev) != NULL && next < cp) {
		assert(next->magic == MAGIC);
		prev = &next->next;
	}

	// Put the new free cell in the list.
	*prev = cp;
	cp->next = next;

#if DEBUG
	while (next != NULL) {
		assert(next->magic == MAGIC);
		next = next->next;
	}
#endif
}

void *realloc(void *op, size_t size) {
	size_t oldSize;
	void *new;

	oldSize = op == NULL ? 0 : obj2cell(op)->size - HDR_SIZE;
	new = malloc(size);
	memcpy(new, op, oldSize > size ? size : oldSize);
	free(op);
	return new;
}


