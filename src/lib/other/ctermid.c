#include <lib.h>
#include <string.h>
#include <stdio.h>


#ifndef L_ctermid
#define L_ctermid	9
#endif

char *ctermid(char *s) {
	static char termid[L_ctermid];

	if (s == NULL)
	  s = termid;
	strcpy(s, "/dev/tty");
	return s;
}
