#include "stdio.h"
#include "stdlib.h"
#include "loc_incl.h"
#include "sys/types.h"

ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int isatty(int fd);
extern void (*_clean)();

static int doWrite(int fd, char *buf, int bytes) {
	int c;

	/* POSIX actually allows write() to return a positive value less
	 * than bytes, so loop...
	 */
	while ((c = write(fd, buf, bytes)) > 0 && c < bytes) {
		bytes -= c;
		buf += c;
	}
	return c > 0;
}

int __flushBuf(int c, FILE *stream) {
	_clean = __cleanup;
	
	if (fileno(stream) < 0)
	  return (unsigned char) c;
	if (! ioTestFlag(stream, _IOWRITE))
	  return EOF;
	if (ioTestFlag(stream, _IOREADING) && ! feof(stream))
	  return EOF;

	stream->_flags &= ~_IOREADING;
	stream->_flags |= _IOWRITING;
	if (! ioTestFlag(stream, _IONBF)) {
		if (! stream->_buf) {
			if (stream == stdout && isatty(fileno(stdout))) {
				if (!(stream->_buf = (unsigned char *) malloc(BUFSIZ))) {
					stream->_flags |= _IONBF;
				} else {
					stream->_flags |= _IOLBF | _IOMYBUF;
					stream->_bufSize = BUFSIZ;
					stream->_count = -1;
				}
			} else {
				if (!(stream->_buf = (unsigned char *) malloc(BUFSIZ))) {
					stream->_flags |= _IONBF;
				} else {
					stream->_flags |= _IOMYBUF;
					stream->_bufSize = BUFSIZ;
					if (! ioTestFlag(stream, _IOLBF))
					  stream->_count = BUFSIZ - 1;
					else
					  stream->_count = -1;
				}
			}
			stream->_ptr = stream->_buf;
		}
	}

	if (ioTestFlag(stream, _IONBF)) {
		char c1 = c;

		stream->_count = 0;
		if (ioTestFlag(stream, _IOAPPEND)) {
			if (lseek(fileno(stream), 0L, SEEK_END) == -1) {
				stream->_flags |= _IOERR;
				return EOF;
			}
		}
		if (write(fileno(stream), &c1, 1) != 1) {
			stream->_flags |= _IOERR;
			return EOF;
		}
		return (unsigned char) c;
	} else if (ioTestFlag(stream, _IOLBF)) {
		*stream->_ptr++ = c;
		/* stream->_count has been updated in putc macro. */
		if (c == '\n' || stream->_count == -stream->_bufSize) {
			int count = -stream->_count;

			stream->_ptr = stream->_buf;
			stream->_count = 0;

			if (ioTestFlag(stream, _IOAPPEND)) {
				if (lseek(fileno(stream), 0L, SEEK_END) == -1) {
					stream->_flags |= _IOERR;
					return EOF;
				}
			}
			if (! doWrite(fileno(stream), (char *) stream->_buf, count)) {
				stream->_flags |= _IOERR;
				return EOF;
			}
		}
	} else {
		int count = stream->_ptr - stream->_buf;

		stream->_count = stream->_bufSize - 1;
		stream->_ptr = stream->_buf + 1;

		if (count > 0) {
			if (ioTestFlag(stream, _IOAPPEND)) {
				if (lseek(fileno(stream), 0L, SEEK_END) == -1) {
					stream->_flags |= _IOERR;
					return EOF;
				}
			}
			if (! doWrite(fileno(stream), (char *) stream->_buf, count)) {
				*(stream->_buf) = c;
				stream->_flags |= _IOERR;
				return EOF;
			}
		}
		*(stream->_buf) = c;
	}
	return (unsigned char) c;
}
