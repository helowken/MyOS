#define nil	0
#include "lib.h"
#include "sys/types.h"
#include "dirent.h"

void rewinddir(DIR *dp) {
	seekdir(dp, 0);
}
