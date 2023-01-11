#include "stdio.h"
#include "stdlib.h"
#include "loc_incl.h"

extern void (*_clean)(void);

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	int retVal = 0;

	_clean = __cleanup;
	if (mode != _IOFBF && mode != _IOLBF && mode != _IONBF)
	  return EOF;

	if (stream->_buf && ioTestFlag(stream, _IOMYBUF))
	  free((void *) stream->_buf);

	stream->_flags &= ~(_IOMYBUF | _IONBF | _IOLBF);

	if (buf && size <= 0)
	  retVal = EOF;
	if (! buf && (mode != _IONBF)) {
		if (size <= 0 || (buf = (char *) malloc(size)) == NULL) 
		  retVal = EOF;
		else
		  stream->_flags |= _IOMYBUF;
	}

	stream->_buf = (unsigned char *) buf;
	stream->_count = 0;
	stream->_flags |= mode;
	stream->_ptr = stream->_buf;

	if (! buf) 
	  stream->_bufSize = 1;
	else
	  stream->_bufSize = size;

	return retVal;
}
