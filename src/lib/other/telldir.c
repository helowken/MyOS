#define nil	0
#include "lib.h"
#include "sys/types.h"
#include "dirent.h"
#include "errno.h"

off_t telldir(DIR *dp) {
	if (dp == nil) {
		errno = EBADF;
		return -1;
	}
	return dp->_pos;
}
