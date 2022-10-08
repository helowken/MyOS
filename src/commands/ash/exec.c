

#include "shell.h"
#include "mail.h"
#include "parser.h"
#include "exec.h"
#include "var.h"
#include "error.h"
#include "errno.h"
#include "limits.h"



/* Called before PATH is changed. The argument is the new value of PATH;
 * pathVal() still returns the old value at this point. Called with
 * interrupts off.
 */
void changePath(char *newVal) {
	char *old, *new;
	int index;
	int firstChange;
	//int builtin;
	
	old = pathVal();
	new = newVal;
	firstChange = 9999;	/* Assume no change */
	index = 0;
	//builtin = -1;
	for (;;) {
		if (*old != *new) {
			firstChange = index;
			if ((*old == '\0' && *new == ':') ||
				(*old == ':' && *new == '\0'))
			  ++firstChange;
			old = new;	/* Ignore subsequent differences */
		}
		if (*new == '\0')
		  break;
		//TODO
	}
}
