#include "sys/types.h"
#include "stdio.h"
#include "loc_incl.h"

ssize_t write(int fd, const char *buf, size_t bytes);
off_t lseek(int fildes, off_t offset, int whence);

int fflush(FILE *stream) {
	int count, i, c1, retVal = 0;
	int adjust;

	if (! stream) {
		for (i = 0; i < FOPEN_MAX; ++i) {
			if (__iotab[i] && fflush(__iotab[i]))
			  retVal = EOF;
		}
		return retVal;
	}

	if (! stream->_buf ||
			(! ioTestFlag(stream, _IOREADING) &&
				! ioTestFlag(stream, _IOWRITING)))
	  return 0;

	if (ioTestFlag(stream, _IOREADING)) {
		adjust = 0;
		if (ioTestFlag(stream, _IOFIFO)) {
			/* Can't seek in a pipe. */
			return 0;
		}

		if (stream->_buf && ! ioTestFlag(stream, _IONBF)) 
		  adjust = -stream->_count;
		stream->_count = 0;

		if (lseek(fileno(stream), (off_t) adjust, SEEK_CUR) == -1) {
			stream->_flags |= _IOERR;
			return EOF;
		}

		if (ioTestFlag(stream, _IOWRITE))
		  stream->_flags &= ~(_IOREADING | _IOWRITING);
		stream->_ptr = stream->_buf;
		return 0;
	} else if (ioTestFlag(stream, _IONBF)) {
		return 0;
	}

	if (ioTestFlag(stream, _IOREAD))	/* "a" or "+" mode */
	  stream->_flags &= ~_IOWRITING;

	count = stream->_ptr - stream->_buf;
	stream->_ptr = stream->_buf;
	if (count <= 0)
	  return 0;

	if (ioTestFlag(stream, _IOAPPEND)) {
		if (lseek(fileno(stream), 0L, SEEK_END) == -1) {
			stream->_flags |= _IOERR;
			return EOF;
		}
	}
	c1 = write(stream->_fd, (char *) stream->_buf, count);
	stream->_count = 0;

	if (count == c1)
	  return 0;

	stream->_flags |= _IOERR;
	return EOF;
}

void __cleanup() {
	register int i;

	for (i = 0; i < FOPEN_MAX; ++i) {
		if (__iotab[i] && ioTestFlag(__iotab[i], _IOWRITING))
		  fflush(__iotab[i]);
	}
}

