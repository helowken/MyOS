

/* Routine for dealing with parsed shell commands.
 */

#include "shell.h"
#include "nodes.h"
#include "memalloc.h"
#include "machdep.h"
#include "mystring.h"

int funcBlockSize;		/* Size of structures in function */
int funcStringSize;		/* Size of strings in node */
pointer funcBlock;		/* Block to allocate function from */
char *funcString;		/* Block to allocate strtings from */

%SIZES

static NodeList *copyNodeList(NodeList *);
static char *nodeSaveStr(char *);
static void sizeNodeList(NodeList *);

static void calcSize(Node *n) {
	%CALC_SIZE
}

static void sizeNodeList(NodeList *lp) {
	while (lp) {
		funcBlockSize += ALIGN(sizeof(NodeList));
		calcSize(lp->node);
		lp = lp->next;
	}
}

static Node *copyNode(Node *n) {
	Node *new;

	%COPY
	return new;
}

static NodeList *copyNodeList(NodeList *lp) {
	NodeList *start;
	NodeList **lpp;

	lpp = &start;
	while (lp) {
		*lpp = funcBlock;
		*(char **) &funcBlock += ALIGN(sizeof(NodeList));
		(*lpp)->node = copyNode(lp->node);
		lp = lp->next;
		lpp = &(*lpp)->next;
	}
	*lpp = NULL;
	return start;
}

static char *nodeSaveStr(char *s) {
	register char *p = s;
	register char *q = funcString;
	char *rv = funcString;

	while ((*q++ = *p++)) {
	}
	funcString = q;
	return rv;
}

/* Free a parse tree.
 */
void freeFunc(Node *n) {
	if (n) 
	  ckFree(n);
}

/* Make a copy of a parse tree.
 */
Node *copyFunc(Node *n) {
	if (n == NULL)
	  return NULL;
	funcBlockSize = 0;
	funcStringSize = 0;
	calcSize(n);
	funcBlock = ckMalloc(funcBlockSize + funcStringSize);
	funcString = (char *) funcBlock + funcBlockSize;
	return copyNode(n);
}
