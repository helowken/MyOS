

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


pointer ckMalloc(int);
pointer ckRealloc(pointer, int);
void free(pointer);		/* Defined in C library */
char *saveStr(char *);
void setStackMark(StackMark *);


#define ckFree(p)	free((pointer) (p))
