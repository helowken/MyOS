#include "lib.h"
#include "unistd.h"

uid_t geteuid() {
	Message msg;

	/* POSIX says that this function is always successful and that no
	 * return value is reserved to indicate an error. Minix syscalls
	 * are not always successful and Minix returns the unreserved value
	 * (uid_t) -1 when there is an error.
	 */
	if (syscall(MM, GETUID, &msg) < 0)
	  return (uid_t) -1;
	return (uid_t) msg.m2_i1;
}
