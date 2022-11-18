

/* Routines to check for mail. 
 */
#include "shell.h"
#include "exec.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "sys/stat.h"

#define MAX_MAIL_BOXES	10

//static int numBoxes;	/* Number of mailboxes */

/* Print appropriate message(s) if mail has arrived. If the argument is
 * nozero, then the value of MAIL has changed, so we just update the
 * values.
 */
void checkMail(int silent) {
	//TODO
}
