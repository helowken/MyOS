#ifndef _UTMP_H 
#define _UTMP_H

#define WTMP	"/usr/adm/wtmp"		/* The login history file */
#define BTMP	"/usr/adm/btmp"		/* The bad-login history file */
#define UTMP	"/etc/utmp"			/* The user accounting file */

struct utmp {
	char ut_user[8];	/* User name */
	char ut_id[4];		/* /etc/inittab ID */
	char ut_line[12];	/* Terminal name */
	char ut_host[16];	/* Host name, when remote */
	short ut_pid;		/* Process id */
	short int ut_type;	/* Type of entry */
	long ut_time;		/* login/logout time */
};

#define ut_name ut_user		/* For compatibility with other systems */

/* Definitions for ut_type. */
#define RUN_LVL			1	/* This is a RUN_LEVEL record */
#define BOOT_TIME		2	/* This is a REBOOT record */
#define INIT_PROCESS	5	/* This process was spawned by INIT */
#define LOGIN_PROCESS	6	/* This is a 'getty' process waiting */
#define USER_PROCESS	7	/* Any other user process */
#define DEAD_PROCESS	8	/* This process has died (wtmp only) */

#endif
