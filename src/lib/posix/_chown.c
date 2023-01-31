#include "lib.h"
#include "string.h"
#include "unistd.h"

int chown(const char *path, uid_t owner, gid_t group) {
	Message msg;

	msg.m1_i1 = strlen(path) + 1;
	msg.m1_i2 = owner;
	msg.m1_i3 = group;
	msg.m1_p1 = (char *) path;
	return syscall(FS, CHOWN, &msg);
}

