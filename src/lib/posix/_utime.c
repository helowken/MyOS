#include <lib.h>
#include <string.h>
#include <utime.h>

int utime(const char *path, const struct utimbuf *times) {
	Message msg;

	if (times == NULL) {
		msg.m2_i1 = 0;		/* Name size 0 means NULL 'times' */
		msg.m2_i2 = strlen(path) + 1;	/* Actual size here */
	} else {
		msg.m2_l1 = times->actime;
		msg.m2_l2 = times->modtime;
		msg.m2_i1 = strlen(path) + 1;
	}
	msg.m2_p1 = (char *) path;
	return syscall(FS, UTIME, &msg);
}
