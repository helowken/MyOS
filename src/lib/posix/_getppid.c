#include <lib.h>
#include <unistd.h>

pid_t getppid() {
	Message msg;

	/* POSIX says that this function is always successful and that no
	 * return value is reserved to indicate an error. Minix syscalls
	 * are not always successful and Minix returns the unreserved value
	 * (pid_t) -1 when there is an error.
	 */
	if (syscall(MM, GETPID, &msg) < 0)
	  return (pid_t) -1;
	return (pid_t) msg.m2_i1;
}
