

#ifndef ALIGN
typedef union {
	int i;
	char *cp;
} Align;
#endif

#define ALIGN(nbytes)	(((nbytes) + sizeof(Align) - 1) & (~(sizeof(Align) - 1)))
