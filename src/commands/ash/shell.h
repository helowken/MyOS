#define JOBS		0
#define SYMLINKS	defined(S_ISLNK)
#define DIRENT		1
#define UDIR		0
#define TILDE		1
#define USEGETPW	0
#define ATTY		0
#define READLINE	0	// TODO 1
#define HASHBANG	0
#define POSIX		1
#define DEBUG		0

typedef void *	pointer;
#ifndef NULL
#define NULL	(void *) 0
#endif

#define MKINIT

#include "sys/types.h"

extern char nullStr[1];		/* Null string */

#ifndef AAA
#include "stdio.h"//TODO
#endif
