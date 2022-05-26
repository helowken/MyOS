#include "minix/com.h"
#include "kernel.h"
#include "stdarg.h"
#include "unistd.h"
#include "stddef.h"
#include "stdlib.h"
#include "signal.h"
#include "proc.h"

#define END_OF_KMESS	-1

void panic(char *msg, int num) {
/* The system has run aground of a fatal kernel error. Terminate execution. */
	static int panicking = 0;

	if (panicking++)
	  return;	/* Prevent recursive panics */

	if (msg != NULL) {
		kprintf("\nKernel panic: %s", msg);
		if (num != NO_NUM)
		  kprintf(" %d", num);
		kprintf("\n", NO_NUM);
	}

	/* Abort MINIX. */
	prepareShutdown(RBT_PANIC);
}

static void kputc(int c) {
/* Accumulate a single character for a kernel message. Send a notification
 * to the output driver if an END_OF_KMESS is encountered.
 */
	if (c != END_OF_KMESS) {
		kernelMsgs.km_buf[kernelMsgs.km_next] = c;	/* Put normal char in buffer */
		if (kernelMsgs.km_size < KMESS_BUF_SIZE)
		  ++kernelMsgs.km_size;
		kernelMsgs.km_next = (kernelMsgs.km_next + 1) % KMESS_BUF_SIZE;
	} else {
		sendSig(OUTPUT_PROC_NR, SIGKMESS);
	}
}

void kprintf(const char *fmt, ...) {
	// TODO
}


