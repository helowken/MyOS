#include <time.h>

char *ctime(const time_t *tp) {
	return asctime(localtime(tp));
}
