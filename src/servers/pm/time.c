#include "pm.h"
#include <minix/callnr.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

static time_t bootTime;

int doTime() {
/* Perform the time(tp) system call. This returns the time in seconds since
 * 1.1.1970. MINIX is an astrophysically naive system that assumes the earth
 * rotates at a constant rate and that such things as leap seconds do not
 * exist.
 */
	register MProc *rmp = currMp;
	clock_t uptime;
	int s;

	if ((s = getUptime(&uptime)) != OK)
	  panic(__FILE__, "doTime couldn't get uptime", s);

	rmp->mp_reply.m_reply_time = (time_t) (bootTime + (uptime / HZ));
	return OK;
}

int doSTime() {
/* Perform the stime(tp) system call. Retrieve the system's uptime (ticks
 * since boot) and store the time in seconds at system boot in the global
 * variable 'bootTime'.
 */
	clock_t uptime;
	int s;

	if (currMp->mp_euid != SUPER_USER)
	  return EPERM;

	if ((s = getUptime(&uptime)) != OK)
	  panic(__FILE__, "doSTime couldn't get uptime", s);
	bootTime = (time_t) inMsg.m_stime - (uptime / HZ);

	/* Also inform FS about the new system time. */
	tellFS(STIME, bootTime, 0, 0);

	return OK;
}

int doTimes() {
/* Perform the times(buf) system call. */
	register MProc *rmp = currMp;
	clock_t t[3];
	int s;

	if ((s = sysTimes(who, t)) != OK)
	  panic(__FILE__, "doTimes couldn't get times", s);

	rmp->mp_reply.m_reply_t1 = t[0];		/* User time */
	rmp->mp_reply.m_reply_t2 = t[1];		/* System time */
	rmp->mp_reply.m_reply_t3 = rmp->mp_child_utime;		/* Child user time */
	rmp->mp_reply.m_reply_t4 = rmp->mp_child_stime;		/* Child system time */
	rmp->mp_reply.m_reply_t5 = t[2];		/* Uptime since boot */

	return OK;
}
