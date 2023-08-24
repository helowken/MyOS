#include <lib.h>
#include <unistd.h>
#include <time.h>

long sysconf(int name) {
	switch (name) {
		case _SC_ARG_MAX:
			return (long) ARG_MAX;
		case _SC_CHILD_MAX:
			return (long) CHILD_MAX;
		case _SC_CLK_TCK:
			return (long) CLOCKS_PER_SEC;
		case _SC_NGROUPS_MAX:
			return (long) NGROUPS_MAX;
		case _SC_OPEN_MAX:
			return (long) OPEN_MAX;
		case _SC_JOB_CONTROL:
			return -1L;		/* No job control */
		case _SC_SAVED_IDS:
			return -1L;		/* No saved uid/gid */
		case _SC_VERSION:
			return (long) _POSIX_VERSION;
		case _SC_STREAM_MAX:
			return (long) STREAM_MAX;
		case _SC_TZNAME_MAX:
			return (long) TZNAME_MAX;
		default:
			errno = EINVAL;
			return -1L;
	}
}
