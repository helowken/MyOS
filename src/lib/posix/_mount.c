#include "lib.h"
#include "string.h"
#include "unistd.h"

int mount(char *special, char *name, int rwFlag) {
	Message msg;

	msg.m1_i1 = strlen(special) + 1;
	msg.m1_i2 = strlen(name) + 1;
	msg.m1_i3 = rwFlag;
	msg.m1_p1 = special;
	msg.m1_p2 = name;
	return syscall(FS, MOUNT, &msg);
}
