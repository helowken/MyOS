


#include "stdio.h"	/* Defines BUFSIZ */
#include "unistd.h"
#include "shell.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "errno.h"


#define OUT_BUF_SIZE	BUFSIZ
#define BLOCK_OUT	-2		/* Output to a fixed block of memory */
#define MEM_OUT	-3			/* Output to dynamically allocated memory */
#define OUTPUT_ERR	01		/* Error occured on output */


Output output = {NULL, 0, NULL, OUT_BUF_SIZE, 1, 0};
Output errOut = {NULL, 0, NULL, 100, 2, 0};
Output memOut = {NULL, 0, NULL, 0, MEM_OUT, 0};
Output *out1 = &output;
Output *out2 = &errOut;


#ifdef mkinit
INCLUDE "output.h"
INCLUDE "memalloc.h"

RESET {
	out1 = &output;
	out2 = &errOut;
	if (memOut.buf != NULL) {
		ckFree(memOut.buf);
		memOut.buf = NULL;
	}
}
#endif


void outStr(register char *p, register Output *file) {
	while (*p) {
		outChar(*p++, file);
	}
}

void out1Str(char *p) {
	outStr(p, out1);
}

void out2Str(char *p) {
	outStr(p, out2);
}

void outFormat(Output *file, char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	doFormat(file, fmt, ap);
	va_end(ap);
}

/* Formatted output. This routine handles a subset of the printf formats:
 *  - Formats supported: d, u, o, X, s, and c.
 *  - The x format is alos accepted but is treated like X.
 *  - The l modifier is accepted.
 *  - The - and # flags and accepted; # only works with the o format.
 *  Width and precision may be specified with any format except c.
 *  An * may be given for the width or precision.
 *  - The obsolete practice of preceding the width with a zero to get
 *    zero padding is not supported; use the precision field.
 *  - A % may be printed by writing %% in the format string.
 */

#define TEMP_SIZE	24

static const char digit[16] = "0123456789ABCDEF";

void doFormat(register Output *dest, register char *fmt, va_list ap) {
	//register char c;
	//TODO
}

void formatStr(char *outBuf, int len, char *fmt, ...) {
	va_list ap;
	Output strOut;

	va_start(ap, fmt);
	strOut.nextChar = outBuf;
	strOut.numLeft = len;
	strOut.fd = BLOCK_OUT;
	strOut.flags = 0;
	doFormat(&strOut, fmt, ap);
	outChar('\0', &strOut);
	if (strOut.flags & OUTPUT_ERR)
	  outBuf[len - 1] = '\0';
	va_end(ap);
}

int xwrite(int fd, char *buf, int bytes) {
	int nTry = 0, i, n;

	n = bytes;
	for (;;) {
		i = write(fd, buf, n);
		if (i > 0) {
			if ((n -= i) <= 0)
			  return bytes;
			buf += i;
			nTry = 0;
		} else if (i == 0) {
			if (++nTry > 10)
			  return bytes - n;
		} else if (errno != EINTR) {
			return -1;
		}
	}
}

void flushOut(Output *dest) {	
	if (dest->buf == NULL || dest->nextChar == dest->buf || dest->fd < 0)
	  return;
	if (xwrite(dest->fd, dest->buf, dest->nextChar - dest->buf) < 0)
	  dest->flags |= OUTPUT_ERR;
	dest->nextChar = dest->buf;
	dest->numLeft = dest->bufSize;
}

static char outJunk[16];

void emptyOutBuf(Output *dest) {
	int offset;

	if (dest->fd == BLOCK_OUT) {
		dest->nextChar = outJunk;
		dest->numLeft = sizeof(outJunk);
		dest->flags |= OUTPUT_ERR;
	} else if (dest->buf == NULL) {
		INTOFF;
		dest->buf = ckMalloc(dest->bufSize);
		dest->nextChar = dest->buf;
		dest->numLeft = dest->bufSize;
		INTON;
	} else if (dest->fd == MEM_OUT) {
		offset = dest->bufSize;
		INTOFF;
		dest->bufSize <<= 1;
		dest->buf = ckRealloc(dest->buf, dest->bufSize);
		dest->numLeft = dest->bufSize - offset;
		dest->nextChar = dest->buf + offset;
		INTON;
	} else {
		flushOut(dest);
	}
	--dest->numLeft;
}




