

/* Routines to check for mail. 
 */
#include "shell.h"
#include "exec.h"
#include "var.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include <sys/stat.h>
#include <stdlib.h>

#define MAX_MAIL_BOXES	10

static int numBoxes;	/* Number of mailboxes */
static time_t mailTime[MAX_MAIL_BOXES];	/* Times of mailBoxes */

/* Print appropriate message(s) if mail has arrived. If the argument is
 * nozero, then the value of MAIL has changed, so we just update the
 * values.
 */
void checkMail(int silent) {
	register int i;
	char *mpath;
	char *p;
	register char *q;
	StackMark stackMark;
	struct stat st;

	if (silent)
	  numBoxes = 10;
	if (numBoxes == 0)
	  return;

	setStackMark(&stackMark);
	mpath = mpathSet() ? mpathVal() : mailVal();
	for (i = 0; i < numBoxes; ++i) {
		p = pathAdvance(&mpath, nullStr);
		if (p == NULL)
		  break;
		if (*p == '\0')
		  continue;
		for (q = p; *q; ++q) {
		}
		if (q[-1] != '/')
		  abort();
		q[-1] = '\0';		/* Delete trailing '/' */
		if (stat(p, &st) < 0)
		  st.st_mtime = 0;
		if (! silent &&
				st.st_size > 0 &&
				st.st_mtime > mailTime[i] &&
				st.st_mtime > st.st_atime) {
			out2Str(pathOpt ? pathOpt : "You have mail");
			out2Char('\n');
		}
		mailTime[i] = st.st_mtime;
	}
	numBoxes = i;
	popStackMark(&stackMark);
}
