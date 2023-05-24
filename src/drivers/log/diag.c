#include <stdio.h>
#include <fcntl.h>

#include "log.h"
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"

int doNewKernelMsgs(Message *msg) {
/* Notification for a new kernel message. */
	KernelMsgs kMsgs;		/* Entire KernelMsgs structure */
	char printBuf[KMESS_BUF_SIZE];		/* Copy new message here */
	static int prevNext = 0;
	int bytes;
	int i, r;

	/* Try to get a fresh copy of the buffer with kernel messages. */
	if ((r = sysGetKernelMsgs(&kMsgs)) != OK) {
		report("LOG", "couldn't get copy of KernelMsgs", r);
		return EDONTREPLY;
	}

	/* Print only the new part. Determine how many new bytes there are with
	 * help of the current and previous 'next' index. Note that the kernel
	 * buffer is circular. This works fine if less than KMESS_BUF_SIZE bytes
	 * is new data; else we miss % KMESS_BUF_SIZE here.
	 * Check for size being positive, the buffer might as well be emptied!
	 */
	if (kMsgs.km_size > 0) {
		bytes = ((kMsgs.km_next + KMESS_BUF_SIZE) - prevNext) % KMESS_BUF_SIZE;
		r = prevNext;		/* Start at previous old */
		i = 0;
		while (bytes > 0) {
			printBuf[i] = kMsgs.km_buf[(r % KMESS_BUF_SIZE)];
			--bytes;
			++r;
			++i;
		}
		/* Now terminate the new message and print it. */
		printBuf[i] = 0;
		printf("%s", printBuf);
		logAppend(printBuf, i);
	}
	
	/* Almost done, store 'next' so that we can determine what part of the
	 * kernel messages buffer to print next time a notification arrives.
	 */
	prevNext = kMsgs.km_next;
	return EDONTREPLY;
}

int doDiagnostics(Message *msg) {
/* The LOG server handles all diagnostic messages from servers and device
 * drivers. It forwards the message to the TTY driver to display it to the
 * user. It also saves a copy in a local buffer so that messages can be
 * reviewed at a later time.
 */
	int result;
	int pNum;
	vir_bytes src;
	int count;
	int i = 0;
	char c;
	static char diagBuf[10240];

	/* Forward the message to the TTY driver. Inform the TTY driver about the
	 * original sender, so that it knows where the buffer to be printed is.
	 * The message type, DIAGNOSTICS, remains the same.
	 */
	if ((pNum = msg->DIAG_PROC_NR) == SELF)
	  msg->DIAG_PROC_NR = pNum = msg->m_source;
	result = sendRec(TTY_PROC_NR, msg);
	
	/* Now also make a copy for the private buffer at the LOG server, so
	 * that the messages can be reviewed at a later time.
	 */
	src = (vir_bytes) msg->DIAG_PRINT_BUF;	
	count = msg->DIAG_BUF_COUNT;
	while (count > 0 && i < sizeof(diagBuf) - 1) {
		if (sysDataCopy(pNum, src, SELF, (vir_bytes) &c, 1) != OK)
		  break;	/* Stop copying on error */
		++src;
		--count;
		diagBuf[i++] = c;
	}
	logAppend(diagBuf, i);

	return result;
}
