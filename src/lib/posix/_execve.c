#define _MINIX_SOURCE

#define nil 0

#include "lib.h"
#include "unistd.h"
#include "string.h"
#include "stddef.h"

/* The frame layout:
 *
 * | null-terminated | ...
   | args and envs   | 
   | ...             | <- stringOff
   -------------------
 * |     0 (nil)     | 
 * |     lastEnvp    | 
 * |     envp2       | 
 * |     envp1       | 
 * |     envp0       | 
 * |     0 (nil)     | 
 * |     lastArgp    | 
 * |     argp2       |  
 * |     argp1       |  
 * |     argp0       |  
 * ------------------- Below is constructed in PM's exec().
 * |     envp        | point to envp0
 * |     argv        | point to argp0
 * |     argc        | argument count
 * |     return      | at the start of main() execution
 */

int execve(const char *path, char * const argv[], char * const envp[]) {
	char * const *ap;
	char * const *ep;
	char *frame;
	char **vp;
	char *sp;
	size_t argc;
	size_t frameSize;
	size_t stringOff;
	size_t n;
	Message msg;
	int overflow;

	/* Create a stack image that only needs to be patched up slightly
	 * by the kernel to be used for the process to be executed.
	 */
	overflow = 0;	
	frameSize = 0;		/* Size of the new initial stack. */
	stringOff = 0;		/* Offset to start of the strings. */
	argc = 0;			/* Argument count. */

	for (ap = argv; *ap != nil; ++ap) {
		n = sizeof(*ap) + strlen(*ap) + 1;	/* argp(i) + argStr(i) + '\0' */
		frameSize += n;
		if (frameSize < 0)
		  overflow = 1;
		stringOff += sizeof(*ap);	/* Offset += argp(i) */
		++argc;
	}

	for (ep = envp; *ep != nil; ++ep) {
		n = sizeof(*ep) + strlen(*ep) + 1;	/* envp(i) + envStr(i) + '\0' */
		frameSize += n;
		if (frameSize < 0)
		  overflow = 1;
		stringOff += sizeof(*ep);	/* Offset += envp(i) */
	}

	/* Add an argument count and two terminating nulls. */
	frameSize += sizeof(argc) + sizeof(*ap) + sizeof(*ep);	
	stringOff += sizeof(argc) + sizeof(*ap) + sizeof(*ep);

	/* Align. */
	frameSize = (frameSize + sizeof(char *) - 1) & ~(sizeof(char *) - 1);

	/* The party is off if there is an overflow. */
	if (overflow || frameSize < 3 * sizeof(char *)) {
		errno = E2BIG;
		return -1;
	}

	/* Allocate space for the stack frame. */
	if ((frame = (char *) sbrk(frameSize)) == (char *) -1) {
		errno = E2BIG;
		return -1;
	}

	/* Set arg count, init pointers to vector and string tables. */
	* (size_t *) frame = argc;
	vp = (char **) (frame + sizeof(argc));
	sp = frame + stringOff;

	/* Load the argument vector and strings. */
	for (ap = argv; *ap != nil; ++ap) {
		*vp++ = (char *) (sp - frame);	/* Load argp(i) */
		n = strlen(*ap) + 1;	
		memcpy(sp, *ap, n);		/* Load argStr(i) */
		sp += n;
	}
	*vp++ = nil;	/* Load nil after lastArgp */

	/* Load the environment vector and strings. */
	for (ep = envp; *ep != nil; ++ep) {
		*vp++ = (char *) (sp - frame);	/* Load envp(i) */
		n = strlen(*ep) + 1;
		memcpy(sp, *ep, n);		/* Load envStr(i) */
		sp += n;
	}
	*vp++ = nil;	/* Load nil after lastEnvp */

	/* Padding. */
	while (sp < frame + frameSize) { 
		*sp++ = 0;
	}

	/* We can finally make the system call. */
	msg.m1_i1 = strlen(path) + 1;
	msg.m1_i2 = frameSize;
	msg.m1_p1 = (char *) path;
	msg.m1_p2 = frame;

	/* Clear unused fields */
	msg.m1_i3 = 0;
	msg.m1_p3 = NULL;

	syscall(MM, EXEC, &msg);

	/* Failure, return the memory used for the frame and exit. */
	sbrk(-frameSize);
	return -1;
}



