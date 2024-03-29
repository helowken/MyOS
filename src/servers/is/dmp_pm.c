#include "is.h"
#include "../pm/mproc.h"
#include <timers.h>

MProc mprocTable[NR_PROCS];

static char *flagsStr(int flags) {
	static char str[10];

	str[0] = (flags & WAITING ? 'W' : '-');
	str[1] = (flags & ZOMBIE ? 'Z' : '-');
	str[2] = (flags & PAUSED ? 'P' : '-');
	str[3] = (flags & ALARM_ON ? 'A' : '-');
	str[4] = (flags & TRACED ? 'T' : '-');
	str[5] = (flags & STOPPED ? 'S' : '-');
	str[6] = (flags & SIGSUSPENDED ? 'U' : '-');
	str[7] = (flags & REPLY ? 'R' : '-');
	str[8] = (flags & ONSWAP ? 'O' : '-');
	str[9] = (flags & SWAPIN ? 'I' : '-');
	str[10] = (flags & DONT_SWAP ? 'D' : '-');
	str[11] = (flags & PRIV_PROC ? 'P' : '-');
	str[12] = '\0';
	return str;
}

void mprocDmp() {
	MProc *mp;
	int i, r, n = 0;
	static int prevIdx = 0;

	printf("Process manager (PM) process table dump\n");

	if ((r = getSysInfo(PM_PROC_NR, SI_PROC_TAB, mprocTable)) != OK) {
		report("IS", "warning: couldn't get copy of pm process table", r);
		return;
	}

	printf("-process- -nr-prnt- -pid/ppid/grp- -uid--gid- -nice- -flags------\n");
	
	for (i = prevIdx; i < NR_PROCS; ++i) {
		mp = &mprocTable[i];
		if (mp->mp_pid == 0 && i != PM_PROC_NR)
		  continue;
		if (++n > 22)
		  break;
		printf("%8.8s %4d%4d  %4d%4d%4d    ",
			mp->mp_name, 
			i, mp->mp_parent, 
			mp->mp_pid, mprocTable[mp->mp_parent].mp_pid, mp->mp_proc_grp);
		printf("%d(%d)  %d(%d)  ",
			mp->mp_ruid, mp->mp_euid, mp->mp_rgid, mp->mp_egid);
		printf(" %3d  %s  ",
			mp->mp_nice, flagsStr(mp->mp_flags));
		printf("\n");
	}
	if (i >= NR_PROCS)
	  i = 0;
	else
	  printf("--more--\r");
	prevIdx = i;
}

void sigactionDmp() {
	MProc *mp;
	int i, r, n = 0;
	static int prevIdx = 0;
	clock_t uptime;

	printf("Process manager (PM) signal action dump\n");

	if ((r = getSysInfo(PM_PROC_NR, SI_PROC_TAB, mprocTable)) != OK) {
		report("IS", "warning: couldn't get copy of pm process table", r);
		return;
	}
	getUptime(&uptime);

	printf("-process- -nr- --ignore- --catch- --block- -tomess-- -pending-- -alarm---\n");
	for (i = prevIdx; i < NR_PROCS; ++i) {
		mp = &mprocTable[i];
		if (mp->mp_pid == 0 && i != PM_PROC_NR)
		  continue;
		if (++n > 22)
		  break;
		printf("%8.8s  %3d  ", mp->mp_name, i);
		printf(" 0x%06x 0x%06x 0x%06x 0x%06x   ",
			mp->mp_sig_ignore, mp->mp_sig_catch, mp->mp_sig_mask,
			mp->mp_sig_to_msg);
		printf("0x%06x  ", mp->mp_sig_pending);

		if (mp->mp_flags & ALARM_ON)
		  printf("%8u", mp->mp_timer.tmr_exp_time - uptime);
		else
		  printf("       -");
		printf("\n");
	}
	if (i >= NR_PROCS)
	  i = 0;
	else
	  printf("--more--\r");
	prevIdx = i;
}



