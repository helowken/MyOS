#include "sysutil.h"

int fKeyCtl(int req, int *fKeys, int *sfKeys) {
/* Send a message to the TTY server to request notifications for function
 * key presses or to disable notifications. Enabling succeeds unless the key
 * is already bound to another process. Disabling only succeeds if the key is
 * bound to the current process.
 */
	Message msg;
	int s;

	msg.FKEY_REQUEST = req;
	msg.FKEY_FKEYS = (fKeys ? *fKeys : 0);
	msg.FKEY_SFKEYS = (sfKeys ? *sfKeys : 0);
	s = taskCall(TTY_PROC_NR, FKEY_CONTROL, &msg);
	if (fKeys)
	  *fKeys = msg.FKEY_FKEYS;
	if (sfKeys)
	  *sfKeys = msg.FKEY_SFKEYS;
	return s;
}


