
#include "loc_incl.h"

char *_i_compute(unsigned long val, int base, char *s, int numDigits) {
	int c;

	c = val % base;
	val /= base;
	if (val || numDigits > 1)
	  s = _i_compute(val, base, s, numDigits - 1);
	*s++ = c > 9 ? 
			c - 10 + 'a' :
			c + '0';
	return s;
}
