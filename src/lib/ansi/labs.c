#include <stdlib.h>

long labs(register long l) {
	return l >= 0 ? l : -l;
}
