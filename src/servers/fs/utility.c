#include "fs.h"
#include "minix/com.h"
#include "unistd.h"
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

int fetchName(char *path, int len, int flag) {
/* Go get path and put it in 'userPath'.
 * If 'flag' = 3 and 'len' <= M3_STRING, the path is present in 'message'.
 * If it is not, go copy it from user space.
 */
	register char *pu, *pm;
	int r;

	/* Check name length for validity. */
	if (len <= 0) {
		errCode = EINVAL;
		return EGENERIC;
	}

	if (len > PATH_MAX) {
		errCode = ENAMETOOLONG;
		return EGENERIC;
	}

	if (flag == M3 && len <= M3_STRING) {
		/* Just copy the path from the message to 'userPath'. */
		pu = userPath;
		pm = inMsg.path_name;	/* Contained in input message */
		do {
			*pu++ = *pm++;
		} while (--len);
		r = OK;
	} else {
		/* String is not contained in the message. Get it from user space. */
		r = sysDataCopy(who, (vir_bytes) path,
				FS_PROC_NR, (vir_bytes) userPath, (phys_bytes) len);
	}
	return r;
}











