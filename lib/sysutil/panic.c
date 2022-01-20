#include "sysutil.h"
#include "stddef.h"

void panic(char *who, char *msg, int num) {
/* Something awful has happened. Panics are caused when an internal
 * inconsistency is detected, e.g., a programming error or illegal
 * value of a defined constant.
 */
	Message m;

	if (who != NULL && msg != NULL) {
		if (num != NO_NUM)
		  printf("Panic in %s: %s: %d\n", who, msg, num);
		else
		  printf("Panic is %s: %s\n", who, msg);
	}

	m.PR_PROC_NR = SELF;
	taskCall(SYSTASK, SYS_EXIT, &m);
}
