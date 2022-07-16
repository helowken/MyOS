#include "fs.h"
#include "minix/com.h"
#include "unistd.h"
#include "file.h"
#include "param.h"

static bool panicking;		/* Inhibits recursive panics durin sync */

int noSys() {
/* Somebody has used an illegal system call number. */
	return EINVAL;
}

void panic(char *who, char *msg, int num) {
/* Something awful has happend. Panics are caused when an internal
 * inconsistency is detected, e.g., a programing error or illegal value of a
 * defined constant.
 */
	if (panicking)
	  return;		/* Do not panic during a sync. */
	panicking = true;		/* Prevent another panic during the sync. */

	printf("FS panic (%s): %s ", who, msg);
	if (num != NO_NUM)
	  printf("%d", num);
	doSync();		/* Flush everything to the disk */
	sysExit(1);
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
