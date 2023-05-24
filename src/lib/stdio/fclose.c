#include <stdio.h>
#include <stdlib.h>
#include "loc_incl.h"

int close(int fd);

int fclose(FILE *fp) {
	register int i, retVal = 0;

	for (i = 0; i < FOPEN_MAX; ++i) {
		if (fp == __iotab[i]) {
			__iotab[i] = 0;
			break;
		}
	}
	if (i >= FOPEN_MAX)
	  return EOF;
	if (fflush(fp))
	  retVal = EOF;
	if (close(fileno(fp)))
	  retVal = EOF;
	if (ioTestFlag(fp, _IOMYBUF) && fp->_buf)
	  free((void *) fp->_buf);
	if (fp != stdin && fp != stdout && fp != stderr)
	  free((void *) fp);
	return retVal;
}
