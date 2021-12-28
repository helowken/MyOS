#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

#define PRIO_MIN	-20
#define PRIO_MAX	20

#define PRIO_PROCESS	0
#define PRIO_PGRP		1
#define PRIO_USER		2

int getpriority(int which, int who);
int setpriority(int which, int who, int prio);

#endif
