

#include "shell.h"
#include "memalloc.h"
#include "error.h"
#include "machdep.h"
#include "mystring.h"
#include "output.h"

static StackBlock stackBase;
static StackBlock *stackBp = &stackBase;
char *stackNext = stackBase.space;
int stackNumLeft = MIN_SIZE;
int stackStrNumLeft;
int hereFd = -1;


/* Like malloc, but returns an error when out of space.
 */
pointer ckMalloc(int bytes) {
	register pointer p;
	pointer malloc();

	if ((p = malloc(bytes)) == NULL)
	  error("Out of space");
	return p;
}

/* Same for realloc.
 */
pointer ckRealloc(register pointer p, int bytes) {
	pointer realloc();
	
	if ((p = realloc(p, bytes)) == NULL)
	  error("Out of space");
	return p;
}

void setStackMark(StackMark *mark) {
	mark->stackBp = stackBp;
	mark->stackNext = stackNext;
	mark->stackNumLeft = stackNumLeft;
}

void popStackMark(StackMark * mark) {
	StackBlock *sp;

	INTOFF;
	while (stackBp != mark->stackBp) {
		sp = stackBp;
		stackBp = sp->prev;
		ckFree(sp);
	}
	stackNext = mark->stackNext;
	stackNumLeft = mark->stackNumLeft;
	INTON;
}

/* Make a copy of a string in safe storage.
 */
char *saveStr(char *s) {
	register char *p;

	p = ckMalloc(strlen(s) + 1);
	scopy(s, p);
	return p;
}

pointer stackAlloc(int bytes) {
	register char *p;
	bytes = ALIGN(bytes);
	if (bytes > stackNumLeft) {
		int blockSize;
		StackBlock *sp;

		blockSize = bytes;
		if (blockSize < MIN_SIZE)
		  blockSize = MIN_SIZE;
		INTOFF;
		sp = ckMalloc(sizeof(StackBlock) - MIN_SIZE + blockSize);
		sp->prev = stackBp;
		stackNext = sp->space;
		stackNumLeft = blockSize;
		stackBp = sp;
		INTON;
	}
	p = stackNext;
	stackNext += bytes;
	stackNumLeft -= bytes;
	return p;
}

/* Called from CHECK_STR_SPACE.
 */
char *makeStrSpace() {
	int len = stackBlockSize() - stackStrNumLeft;
	growStackBlock();
	stackStrNumLeft = stackBlockSize() - len;
	return stackBlock() + len;
}

/* When the parser reads in a string, it wants to stick the string on the
 * stack and only adjust the stack pointer when it knows how big the
 * string is. StackBlock returns a pointer to a block of space on top of 
 * the stack and stackBlockLen returns the length of this block. 
 * GrowStackBlock will grow this space by at least one byte, possibly 
 * moving it (like realloc). GrabStackBlock actually allocates the part 
 * of the block that has been used.
 */
void growStackBlock() {
	char *p;
	int newLen = stackNumLeft * 2 + 100;
	char *oldSpace = stackNext;
	int oldLen = stackNumLeft;
	StackBlock *sp;

	/* StackBase can't be realloc since it is statically allocated */
	if (stackNext == stackBp->space && stackBp != &stackBase) {
		INTOFF;
		sp = stackBp;
		stackBp = sp->prev;
		sp = ckRealloc((pointer) sp, sizeof(StackBlock) - MIN_SIZE + newLen);
		sp->prev = stackBp;
		stackBp = sp;
		stackNext = sp->space;
		stackNumLeft = newLen;
		INTON;
	} else {
		/* A new stackBlock will be created since newLen > stackNumLeft */
		p = stackAlloc(newLen);
		bcopy(oldSpace, p, oldLen);
		stackNext = p;				/* Free the space */
		stackNumLeft += newLen;		/* We just allocated */
	}
}

void grabStackBlock(int len) {
	len = ALIGN(len);
	stackNext += len;
	stackNumLeft -= len;
}

char *growStackStr() {
	int len = stackBlockSize();
	if (hereFd >= 0 && len >= 1024) {
		xwrite(hereFd, stackBlock(), len);
		stackStrNumLeft = len - 1;
		return stackBlock();
	}
	growStackBlock();
	stackStrNumLeft = stackBlockSize() - len - 1;
	return stackBlock() + len;
}

void ungrabStackStr(char *s, char *p) {
	stackNumLeft += stackNext - s;
	stackNext = s;
	stackStrNumLeft = stackNumLeft - (p - s);
}
