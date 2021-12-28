#include "lib.h"
#include "sys/time.h"
#include "time.h"

int settimeofday(const struct timeval *tv, const void *tz) {
	return stime(&tv->tv_sec);
}
