#include "lib.h"
#include "unistd.h"

gid_t getegid() {
	Message msg;

	/* POSIX says that this function is always successful and that no
	 * return value is reserved to indicate an error. Minix syscalls
	 * are not always successful and Minix returns the unreserved value
	 * (gid_t) -1 when there is an error.
	 */
	if (syscall(MM, GETGID, &msg) < 0)
	  return (gid_t) -1;
	return (gid_t) msg.m2_i1;
}
