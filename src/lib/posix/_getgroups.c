#include <lib.h>
#include <unistd.h>
#include <time.h>

int getgroups(int size, gid_t groupList[]) {
/* Not supported in MINIX. */
	return 0;
}
