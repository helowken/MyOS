#include "sysutil.h"

void report(char *who, char *msg, int num) {
/* Display a message for a server. */
	if (num != NO_NUM) 
	  printf("%s %s %d\n", who, msg, num);
	else
	  printf("%s: %s\n", who, msg);
}
