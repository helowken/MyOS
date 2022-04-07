#ifndef _PTRACE_H
#define _PTRACE_H

#define T_STOP		-1	/* Stop the process */
#define T_OK		0	/* Enable tracing by parent for this process */
#define T_GETINS	1	/* Return value from instruction space */
#define T_GETDATA	2	/* Return value from data space */
#define T_GETUSER	3	/* Return value from user process table */
#define T_SETINS	4	/* Set value from instruction space */
#define T_SETDATA	5	/* Set value from data space */
#define T_SETUSER	6	/* Set value in user process table */
#define T_RESUME	7	/* Resume execution */
#define	T_EXIT		8	/* Exit */
#define T_STEP		9	/* Set trace bit */

long ptrace(int req, pid_t pid, long addr, long data);

#endif
