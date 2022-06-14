/* A server must occasionally print some message. It uses a simple version of
 * printf() found in the system lib that calls kputc() to output characters.
 * Printing is done with a call to the kernel, and not by going through FS.
 *
 * This rouine can only be used by servers and device drivers. The kernel
 * must define its own kputc(). Note that the log driver also defines its own
 * kputc() to directly call the TTY instead of going through this library.
 */

#include "sysutil.h"

__attribute__((weak)) int outputPNum = OUTPUT_PROC_NR;

void kputc(int c) {
/* Accumulate another character. If 0 or buffer full, print it. */
	static int bufCount;		/* # characters in the buffer */
	static char printBuf[80];	/* Output is buffered here */
	Message msg;

	if ((c == 0 && bufCount > 0) || bufCount == sizeof(printBuf)) {
		/* Send the buffer to the OUTPUT_PROC_NR driver. */
		msg.DIAG_BUF_COUNT = bufCount;
		msg.DIAG_PRINT_BUF = printBuf;
		msg.DIAG_PROC_NR = SELF;
		msg.m_type = DIAGNOSTICS;
		sendRec(outputPNum, &msg);
		bufCount = 0;

		/* If the output fails, e.g., due to an ELOCKED, do not retry output
		 * at the FS as if this were a normal user-land printf(). This may
		 * result in even worse problems.
		 */
	} 
	if (c != 0) {
		/* Append a simple character to the output buffer. */
		printBuf[bufCount++] = c;
	}
}
