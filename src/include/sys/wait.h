/* The value returned by wait() and waitpid() depends on whether 
 * the process terminated by an exit() call, was killed by a signal,
 * or was stopped due to job control, as follows:
 *
 *							  High byte   Low byte
 *				+-----------------------------------
 *	exit(status)			|	status	|	 0		|
 *				+-----------------------------------
 *	killed by signal		|	  0		|  signal	|
 *				+-----------------------------------
 *	stopped (job control)	|	signal	|	0177	|
 *				+-----------------------------------
 *
 */

#ifndef _WAIT_H
#define	_WAIT_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

#define	_LOW(v)		( (v) & 0377 )
#define _HIGH(v)	( ((V) >> 8) & 0377)

#define WNOHANG		1	/* Do not wait for child to exit */
#define WUNTRACED	2	/* For job control; not implemented */

#define WIFEXITED(s)	(_LOW(s) == 0)		/* Normal exit */
#define WEXITSTATUS(s)	(_HIGH(s))			/* Exit status */
#define WIFSIGNALED(s)	(((unsigned int)(s) - 1 & 0xFFFF) < 0xFF)	/* Signaled */
#define WTERMSIG(s)		(_LOW(s) & 0177)	/* Sig value */
#define WIFSTOPPED(s)	(_LOW(s) == 0177)	/* Stopped */
#define WSTOPSIG(s)		(_HIGH(s) & 0377)	/* Stop signal */

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

#endif
