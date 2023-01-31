#include "pm.h"
#include "minix/callnr.h"
#include "signal.h"
#include "mproc.h"
#include "param.h"

int doGetSet() {
/* Handle GETUID, GETGID, GETPID, GETPGPR, SETUID, SETGID, SETSID. The four
 * GETs and SETSID return their primary results in 'r'. GETUID, GETGID, and
 * GETPID also return secondary results (the effective IDs, or the parent
 * process ID) in 'm_reply_res2', which is returned to the user.
 */

	register MProc *rmp = currMp;
	register int r;
	uid_t uid;
	gid_t gid;

	switch (callNum) {
		case GETUID:
			r = rmp->mp_ruid;
			rmp->mp_reply.m_reply_res2 = rmp->mp_euid;
			break;
		case GETGID:
			r = rmp->mp_rgid;
			rmp->mp_reply.m_reply_res2 = rmp->mp_egid;
			break;
		case GETPID:
			r = rmp->mp_pid;
			rmp->mp_reply.m_reply_res2 = mprocTable[rmp->mp_parent].mp_pid;
			break;
		case SETUID:
			uid = (uid_t) inMsg.m_user_id;
			if (rmp->mp_ruid != uid && rmp->mp_euid != SUPER_USER)
			  return EPERM;
			rmp->mp_ruid = uid;
			rmp->mp_euid = uid;
			tellFS(SETUID, who, uid, uid);
			r = OK;
			break;
		case SETGID:
			gid = (gid_t) inMsg.m_group_id;
			if (rmp->mp_rgid != gid && rmp->mp_euid != SUPER_USER)
			  return EPERM;
			rmp->mp_rgid = gid;
			rmp->mp_egid = gid;
			tellFS(SETGID, who, gid, gid);
			r = OK;
			break;
		case SETSID:
			if (rmp->mp_proc_grp == rmp->mp_pid)
			  return EPERM;
			rmp->mp_proc_grp = rmp->mp_pid;
			tellFS(SETSID, who, 0, 0);
			/* Fall through */
		case GETPGRP:
			r = rmp->mp_proc_grp;
			break;
		default:
			r = EINVAL;
			break;
	}
	return r;
}
