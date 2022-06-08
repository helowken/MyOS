#include "minix/com.h"
#include "kernel.h"
#include "stdarg.h"
#include "unistd.h"
#include "stddef.h"
#include "stdlib.h"
#include "signal.h"
#include "proc.h"

#define END_OF_KMESS	-1

void panic(const char *msg, int num) {
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

void kprintf(const char *fmt, ...) {	/* Format to be printed */
	int c;		/* Next character in fmt */
	int d;
	unsigned long u;		/* Hold number argument */
	int base;		/* Base of number arg */
	int negative = 0;		/* Print minus sign */
	static char x2c[] = "0123456789ABCDEF";		/* Number conversion table */
	char ascii[8 * sizeof(long) / 3 + 2];		/* String for ascii number */
	char *s = NULL;		/* String to be printed */
	va_list argp;		/* Optional arguments */

	va_start(argp, fmt);		/* init variable arguments */

	while ((c = *fmt++) != 0) {
		if (c == '%') {		/* Expect format '%key' */
			switch (c = *fmt++) {	/* Determine what to do */

			/* Known keys are %d, %u, %x, %s, and %%. This is easily extended
			 * with number types like %b and %o by providing a different base.
			 * Number type keys don't set a string to 's', but use the general
			 * conversion after the switch statement.
			 */
				case 'd':		/* Output decimal */
					d = va_arg(argp, signed int);
					if (d < 0) {
						negative = 1; 
						u = -d;
					} else {
						u = d;
					}
					base = 10;
					break;
				case 'u':		/* Output unsigned long */
					u = va_arg(argp, unsigned long);
					base = 10;
					break;
				case 'x':		/* Output hexadecimal */
					u = va_arg(argp, unsigned long);
					base = 0x10;
					break;
				case 's':		/* Output string */
					s = va_arg(argp, char *);
					if (s == NULL)
					  s = "(null)";
					break;
				case '%':		/* Output percent */
					s = "%";
					break;

				/* Unrecognized key. */
				default:		/* Echo back %key */
					s = "%?";	
					s[1] = c;	/* Set unknown key */
			}

			/* Assume a number if no string is set. Convert to ascii. */
			if (s == NULL) {
				s = ascii + sizeof(ascii) - 1;
				*s = 0;
				do {
					*--s = x2c[(u % base)];	
				} while ((u /= base) > 0);
			}

			/* This is where the actual output for format "%key" is done. */
			if (negative) 
			  kputc('-');		/* Print sign if negative */
			while (*s != 0) {
				kputc(*s++);		/* Print string/number */
			}
			s = NULL;		/* Reset for next round */
		} else {
			kputc(c);		/* Print and continue */
		}
	} 	
	kputc(END_OF_KMESS);	/* Terminate output */
	va_end(argp);
}



