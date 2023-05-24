#include <stdio.h>

#if (SEEK_CUR != 1) || (SEEK_END != 2) || (SEEK_SET != 0)
#error SEEK_* values are wrong
#endif

#include "loc_incl.h"
#include <sys/types.h>

off_t lseek(int fd, off_t offset, int whence);

int fseek(FILE *stream, long offset, int whence) {
	int adjust = 0;
	long pos;

	/* Clear both the end of file and error flags. */
	stream->_flags &= ~(_IOEOF | _IOERR);

	if (ioTestFlag(stream, _IOREADING)) {
		if (whence == SEEK_CUR &&
				stream->_buf &&
				! ioTestFlag(stream, _IONBF)) {
			adjust = stream->_count;
		}
		stream->_count = 0;
	} else if (ioTestFlag(stream, _IOWRITING)) {
		fflush(stream);
	} else {
		/* Neither reading nor writing. The buffer must be empty. */
	}

	pos = lseek(fileno(stream), offset - adjust, whence);
	if (ioTestFlag(stream, _IOREAD) && 
		ioTestFlag(stream, _IOWRITE))
	  stream->_flags &= ~(_IOREADING | _IOWRITING);

	stream->_ptr = stream->_buf;
	return pos == -1 ? -1 : 0;
}
