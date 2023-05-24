#include <time.h>

double difftime(time_t time1, time_t time0) {
	/* Be careful: time_t may be unsigned */
	if ((time_t) -1 > 0 && time0 > time1)
	  return - (double) (time0 - time1);
	return (double) (time1 - time0);
}
