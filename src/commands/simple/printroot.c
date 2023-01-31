/* This program figures out what the root device is by doing a stat on it, and
 * then searching /dev until it finds an entry with the same device number.
 * A typical use (probably the only use) is in /etc/rc for initializing
 * /etc/mtab, as follows:
 *
 *	/usr/bin/printroot > /etc/mtab
 */

#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "limits.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "minix/minlib.h"
#include "dirent.h"

static char DEV_PATH[] = "/dev/";	
static char MESSAGE[] = " / ";
#define UNKNOWN_DEV	"/dev/unknown"
#define ROOT		"/"
static int rflag;	/* Print only the root device, not a full mtab line. */

static void done(char *name, int status) {
	int v;

	tell(name);
	if (rflag) {
		tell("\n");
	} else {
		tell(MESSAGE);
		v = fsVersion(name, "printroot");	/* Determine file system version */
		if (v == 3)
		  tell("3 rw\n");
		else
		  tell("0 rw\n");
	}
	exit(status);
}

int main(int argc, char **argv) {
	DIR *dp;
	struct dirent *entry;
	struct stat fileStat, rootStat;
	static char nameBuf[sizeof(DEV_PATH) + NAME_MAX];

	rflag = (argc > 1 && strcmp(argv[1], "-r") == 0);

	if (stat(ROOT, &rootStat) == 0 &&
			(dp = opendir(DEV_PATH)) != (DIR *) NULL) {
		while ((entry = readdir(dp)) != (struct dirent *) NULL) {
			strcpy(nameBuf, DEV_PATH);
			strcat(nameBuf, entry->d_name);
			if (stat(nameBuf, &fileStat) != 0 ||
					! S_ISBLK(fileStat.st_mode) ||
					fileStat.st_rdev != rootStat.st_dev)
			  continue;

			done(nameBuf, 0);
		}
	}
	done(UNKNOWN_DEV, 1);
	return 0;	/* Not reached. */
}

