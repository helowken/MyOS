#include "fs.h"
#include "minix/com.h"
#include "unistd.h"
#include "file.h"
#include "param.h"

void panic(char *who, char *msg, int num) {
	// TODO
}

time_t clockTime() {
/* This routine returns the time in secnods since 1.1.1970. MINIX is an
 * astrophysically naive system that assumes the earth rotates at a constant
 * rate and that such things as leap seconds do not exist.
 */
	register int r;
	clock_t uptime;

	if ((r = getUptime(&uptime)) != OK)
	  panic(__FILE__, "clockTime err", r);

	return (time_t) (bootTime + (uptime / HZ));
}
