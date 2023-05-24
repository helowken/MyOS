/*
 * Fill a buffer.
 */

#if defined(POSIX_SOURCE)
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "loc_incl.h"

ssize_t read(int fd, void *buf, size_t n);

int __fillBuf(register FILE *stream) {
	static unsigned char ch[FOPEN_MAX];
	register int i;

	stream->_count = 0;
	if (fileno(stream) < 0)
	  return EOF;
	if (ioTestFlag(stream, (_IOEOF | _IOERR)))
	  return EOF;
	if (! ioTestFlag(stream, _IOREAD)) {
		stream->_flags |= _IOERR;
		return EOF;
	}
	if (ioTestFlag(stream, _IOWRITING)) {
		stream->_flags |= _IOERR;
		return EOF;
	}

	if (! ioTestFlag(stream, _IOREADING))
	  stream->_flags |= _IOREADING;

	if (! ioTestFlag(stream, _IONBF) && ! stream->_buf) {
		stream->_buf = (unsigned char *) malloc(BUFSIZ);
		if (! stream->_buf) {
			stream->_flags |= _IONBF;
		} else {
			stream->_flags |= _IOMYBUF;
			stream->_bufSize = BUFSIZ;
		}
	}

	/* Flush line-buffered output when filling an input buffer */
	for (i = 0; i < FOPEN_MAX; ++i) {
		if (__iotab[i] && ioTestFlag(__iotab[i], _IOLBF)) {
			if (ioTestFlag(__iotab[i], _IOWRITING))
			  fflush(__iotab[i]);
		}
	}

	if (! stream->_buf) {
		stream->_buf = &ch[fileno(stream)];
		stream->_bufSize = 1;
	}
	stream->_ptr = stream->_buf;
	stream->_count = read(stream->_fd, (char *) stream->_buf, stream->_bufSize);

	if (stream->_count <= 0) {
		if (stream->_count == 0) 
		  stream->_flags |= _IOEOF;
		else
		  stream->_flags |= _IOERR;
		return EOF;
	}
	--stream->_count;

	return *stream->_ptr++;
}
