

#include "shell.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"

static StackBlock stackBase;
static StackBlock *stackBp = &stackBase;
char *stackNext = stackBase.space;
int stackNumLeft = MIN_SIZE;


/* Like malloc, but returns an error when out of space.
 */
pointer ckMalloc(int bytes) {
	register pointer p;
	pointer malloc();

	if ((p = malloc()) == NULL)
	  error("Out of space");
	return p;
}

void setStackMark(StackMark *mark) {
	mark->stackBp = stackBp;
	mark->stackNext = stackNext;
	mark->stackNumLeft = stackNumLeft;
}

/* Make a copy of a string in safe storage.
 */
char *saveStr(char *s) {
	register char *p;

	p = ckMalloc(strlen(s) + 1);
	scopy(s, p);
	return p;
}


