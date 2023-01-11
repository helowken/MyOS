

#define MIN_SIZE	504		/* Minimum size of a block */

typedef struct StackBlock {
	struct StackBlock *prev;
	char space[MIN_SIZE];
} StackBlock;

typedef struct {
	StackBlock *stackBp;
	char *stackNext;
	int stackNumLeft;
} StackMark;

extern char *stackNext;
extern int stackNumLeft;
extern int stackStrNumLeft;
extern int hereFd;

pointer ckMalloc(int);
pointer ckRealloc(pointer, int);
void free(pointer);		/* Defined in C library */
char *saveStr(char *);
void setStackMark(StackMark *);
void popStackMark(StackMark *);
void growStackBlock();
void grabStackBlock(int);
char *growStackStr();
char *makeStrSpace();
pointer stackAlloc(int);
void stackUnalloc(pointer);
void ungrabStackStr(char *, char *);


#define stackBlock()			stackNext
#define stackBlockSize()		stackNumLeft
#define START_STACK_STR(p)		p = stackBlock(), stackStrNumLeft = stackBlockSize()
#define ST_PUTC(c, p)			(--stackStrNumLeft >= 0 ? (*p++ = (c)) : (p = growStackStr(), *p++ = (c)))
#define CHECK_STR_SPACE(n, p)	if (stackStrNumLeft < n) p = makeStrSpace(); else
#define UST_PUTC(c, p)			(--stackStrNumLeft, *p++ = (c))
#define STACK_STR_NUL(p)		(stackStrNumLeft == 0 ? (p = growStackStr(), *p = '\0') : (*p = '\0'))
#define ST_UNPUTC(p)			(++stackStrNumLeft, --p)
#define ST_ADJUST(amount, p)	(p += (amount), stackStrNumLeft -= (amount))
#define grabStackStr(p)			stackAlloc(stackBlockSize() - stackStrNumLeft)

#define ckFree(p)	free((pointer) (p))
