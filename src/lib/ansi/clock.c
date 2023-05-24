#include <time.h>
#include <sys/times.h>

clock_t clock() {
	struct tms tms;

	times(&tms);
	return tms.tms_utime;
}
