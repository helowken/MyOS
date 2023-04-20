#include "lib.h"
#include "sys/stat.h"
#include "dirent.h"
#include "fcntl.h"
#include "stddef.h"
#include "string.h"
#include "unistd.h"

static char base[] = "/dev";
static char path[sizeof(base) + 1 + NAME_MAX];

char *ttyname(int fd) {
	DIR *devices;
	struct dirent *entry;
	struct stat ttyStat;
	struct stat devStat;

	/* Simple first test: file descriptor must be a character device */
	if (fstat(fd, &ttyStat) < 0 || ! S_ISCHR(ttyStat.st_mode))
	  return (char *) NULL;

	/* Open device directory for reading */
	if ((devices = opendir(base)) == (DIR *) NULL)
	  return (char *) NULL;

	/* Scan the entries for one that matches perfectly */
	while ((entry = readdir(devices)) != (struct dirent *) NULL) {
		if (ttyStat.st_ino != entry->d_ino)
		  continue;
		strcpy(path, base);
		strcat(path, "/");
		strcat(path, entry->d_name);
		if (stat(path, &devStat) < 0 || ! S_ISCHR(devStat.st_mode))
		  continue;
		if (ttyStat.st_ino == devStat.st_ino &&
				ttyStat.st_dev == devStat.st_dev &&
				ttyStat.st_rdev == devStat.st_rdev) {
			closedir(devices);
			return path;
		}
	}

	closedir(devices);
	return (char *) NULL;
}
