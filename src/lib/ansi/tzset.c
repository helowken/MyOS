#include <time.h>
#include "loc_time.h"

void tzset() {
	_tzset();
}
