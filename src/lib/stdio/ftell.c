#include <stdio.h>

#if (SEEK_CUR != 1) || (SEEK_END != 2) || (SEEK_SET != 0)
#error SEEK_* values are wrong
#endif

#include "loc_incl.h"
#include <sys/types.h>

off_t lseek(int fd, off_t offset, int whence);

long ftell(FILE *stream) {
	long result;
	int adjust = 0;

	if (ioTestFlag(stream, _IOREADING))
	  adjust = -stream->_count;
	else if (ioTestFlag(stream, _IOWRITING) &&
			stream->_buf &&
			! ioTestFlag(stream, _IONBF))
	  adjust = stream->_ptr - stream->_buf;

	result = lseek(fileno(stream), (off_t) 0, SEEK_CUR);

	if (result == -1)
	  return result;

	result += (long) adjust;
	return result;
}
