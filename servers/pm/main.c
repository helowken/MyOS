#include "pm.h"
#include "minix/callnr.h"
#include "signal.h"
#include "stdlib.h"
#include "sys/resource.h"
#include "string.h"
#include "mproc.h"
#include "param.h"

#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"

#define clickToRoundKB(n)	\
	((unsigned) ((((unsigned long) (n) << CLICK_SHIFT) + 512) / 1024))


void setReply(int pNum, int result) {
/* Fill in a reply message to be sent later to a user process. System calls
 * may occasionally fill in other fields, this is only for the main return
 * value, and for setting the "must send reply" flag.
 */
	register MProc *rmp = &mprocTable[pNum];

	rmp->mp_reply.reply_res = result;
	rmp->mp_flags |= REPLY;		/* Reply pending */

	if (rmp->mp_flags & ONSWAP) 
	  swapInQueue(rmp);		/* Must swap this process back in */
}

static void patchMemChunks(Memory *memChunks, MemMap *memMap) {
/* Remove server memory from the free memory list. The boot monitor
 * promises to put processes at the start of memory chunks. The
 * tasks all use same base address, so only the first task changes
 * the memory lists. The servers and init have their own memory
 * spaces and their memory will be remvoed from the list.
 */
	Memory *memp;
	for (memp = memChunks; memp < &memChunks[NR_MEMS]; ++memp) {
		if (memp->base == memMap[T].physAddr) {
			memp->base += memMap[D].len;
			memp->size -= memMap[D].len;
			return;
		}
	}
}

static void getMemChunks(Memory *memChunks) {
/* Initialize the free memory list from 'memory' boot variable. */
	long base, size, limit;
	char *s, *end;
	int i, done = 0;
	Memory *memp;

	/* Initialize everything to zero. */
	for (i = 0; i < NR_MEMS; ++i) {
		memp = &memChunks[i];
		memp->base = memp->size = 0;
	}

	/* The available memory is determined by MINIX' boot loader as a list of
	 * (base:size)-pairs in boot.c and boothead.s. The 'memory' boot variable 
	 * is set in boot.c. The format is "b0:s0,b1:s1,b2:s2", where b0:s0 is 
	 * low mem, b1:s1 is mem between 1M and 16M, b2:s2 is mem above 16M. Pairs 
	 * b1:s1 and b2:s2 are combined if the memory is adjacent.
	 */
	s = findParam("memory");	/* Get memory boot variable */
	for (i = 0; i < NR_MEMS && !done; ++i) {
		memp = &memChunks[i];
		base = size = 0;
		if (*s != 0) {		
			/* Read fresh base and expect colon as next char. */
			base = strtoul(s, &end, 16); 	
			if (end != s && *end == ':')
			  s = ++end;		/* Skip ':' */
			else
			  *s = 0;			/* Terminate, should not happend. */

			/* Read fresh size and expect comma or assume end. */
			size = strtoul(s, &end, 16);
			if (end != s && *end == ',')
			  s = ++end;		/* Skip ';' */
			else
			  done = 1;

		}
		limit = base + size;
		base = (base + CLICK_SIZE - 1) & ~(long)(CLICK_SIZE - 1);
		limit &= ~(long)(CLICK_SIZE - 1);
		if (limit <= base)
		  continue;
		memp->base = base >> CLICK_SHIFT; 
		memp->size = (limit - base) >> CLICK_SHIFT;
	}
}

static int getNiceValue(int queue) {
/* Processes in the boot image have a priority assigned. The PM doesn't know
 * about priorities, but uses 'nice' values instead. The priority is between
 * MIN_USER_Q and MAX_USER_Q. We have to scale between PRIO_MIN and PRIO_MAX.
 */
	int niceVal = (queue - USER_Q) * (PRIO_MAX - PRIO_MIN + 1) /
					(MIN_USER_Q - MAX_USER_Q + 1);
	if (niceVal > PRIO_MAX)
	  niceVal = PRIO_MAX;
	if (niceVal < PRIO_MIN)
	  niceVal = PRIO_MIN;
	return niceVal;
}

static void pmInit() {
/* Initialize the process manager. */
	int s, i;
	static BootImage images[NR_BOOT_PROCS];
	register BootImage *ip;
	static char coreSigs[] = { SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
		SIGFPE, SIGUSR1, SIGSEGV, SIGUSR2 };
	static char ignoreSigs[] = { SIGCHLD };
	register MProc *rmp;
	register char *sigPtr;
	Message msg;
	Memory memChunks[NR_MEMS];
	MemMap memMap[NR_LOCAL_SEGS];
	phys_clicks minixClicks, totalClicks, freeClicks;

	/* Init calls. */
	initSysCalls();
		
	/* Initialize process table, including timers. */
	for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
		timerInit(&rmp->mp_timer);
	}

	/* Build the set of signals which cause core dumps, and the set of signals
	 * that are by default ignored.
	 */
	sigemptyset(&coreSigSet);
	for (sigPtr = coreSigs; sigPtr < coreSigs + sizeof(coreSigs); ++sigPtr) {
		sigaddset(&coreSigSet, *sigPtr);
	}
	sigemptyset(&ignoreSigSet);
	for (sigPtr = ignoreSigs; sigPtr < ignoreSigs + sizeof(ignoreSigs); ++sigPtr) {
		sigaddset(&ignoreSigSet, *sigPtr);
	}

	/* Obtain a copy of the boot monitor parameters and the kernel info struct.
	 * Parse the list of free memory chunks. This list is what the boot monitor
	 * reported, but it must be corrected for the kernel and system processes.
	 */
	if ((s = sysGetMonParams(monitorParams, sizeof(monitorParams))) != OK)
	  panic(__FILE__, "get monitor params failed", s);
	getMemChunks(memChunks);
	if ((s = sysGetKernelInfo(&kernelInfo)) != OK)
	  panic(__FILE__, "get kernel info failed", s);
	
	/* Get the memory map of the kernel to see how much memory it uses. */
	if ((s = getMemMap(SYSTASK, memMap)) != OK)
	  panic(__FILE__, "couldn't get memory map of SYSTASK", s);
	minixClicks = memMap[S].physAddr + memMap[S].len - memMap[T].physAddr;
	patchMemChunks(memChunks, memMap);

	/* Initialize PM's process table. Request a copy of the system image table
	 * that is defined at the kernel level to see which slots to fill in. 
	 */
	if ((s = sysGetImage(images)) != OK)
	  panic(__FILE__, "couldn't get image table: %d\n", s);
	procsInUse = 0;		/* Start populating table */
	printf("Building process table:");		/* Show what's happening */
	//for (i = 0; i < NR_BOOT_PROCS; ++i) { // TODO
	for (i = 0; i < 6; ++i) {
		ip = &images[i];
		if (ip->pNum >= 0) {
			++procsInUse;		/* Found user process */

			/* Set process details found in the image table. */
			rmp = &mprocTable[ip->pNum];
			strncpy(rmp->mp_name, ip->procName, PROC_NAME_LEN);
			rmp->mp_parent = RS_PROC_NR;
			rmp->mp_nice = getNiceValue(ip->priority);
			if (ip->pNum == INIT_PROC_NR) {	/* User Process */
				rmp->mp_pid = INIT_PID;
				rmp->mp_flags |= IN_USE;
				sigemptyset(&rmp->mp_sig_ignore);
			} else {		/* System process */
				rmp->mp_pid = getFreePid();
				rmp->mp_flags |= IN_USE | DONT_SWAP | PRIV_PROC;
				sigfillset(&rmp->mp_sig_ignore);
			}
			sigemptyset(&rmp->mp_sig_mask);
			sigemptyset(&rmp->mp_sig_catch);
			sigemptyset(&rmp->mp_sig_to_msg);

			/* Get memory map for this process from the kernel. */
			if ((s = getMemMap(ip->pNum, rmp->mp_memmap)) != OK)
			  panic(__FILE__, "couldn't get process entry", s);
			minixClicks = rmp->mp_memmap[S].physAddr + 
				rmp->mp_memmap[S].len - rmp->mp_memmap[T].physAddr;
			patchMemChunks(memChunks, rmp->mp_memmap);

			/* Tell FS about this system process. */
			msg.PR_PROC_NR = ip->pNum;
			msg.PR_PID = rmp->mp_pid;
			if ((s = send(FS_PROC_NR, &msg)) != OK)
			  panic(__FILE__, "can't sync up with FS", s);

			printf(" %s", ip->procName);	/* Display process name */
		}
	}
	printf(".\n");

	/* Override some details. PM is somewhat special. */
	mprocTable[PM_PROC_NR].mp_pid = PM_PID;
	mprocTable[PM_PROC_NR].mp_parent = PM_PROC_NR;	/* PM doesn't have parent */

	/* Tell FS that no more system processes follow and synchronize. */
	msg.PR_PROC_NR = NONE;
	if (sendRec(FS_PROC_NR, &msg) != OK || msg.m_type != OK)
	  panic(__FILE__, "can't sync up with FS", NO_NUM);
	
	// TODO bootDev
	
	/* Initialize tables to all physical memory and print memory information. */
	printf("Physical memory:");
	initMemory(memChunks, &freeClicks);
	totalClicks = minixClicks + freeClicks;
	printf(" total %u KB,", clickToRoundKB(totalClicks));
	printf(" system %u KB,", clickToRoundKB(minixClicks));
	printf(" free %u KB,", clickToRoundKB(freeClicks));
}

static void getWork() {
/* Wait for the next message and extract useful information from it. */
	if (receive(ANY, &inputMsg) != OK)
	  panic(__FILE__, "PM receive error", NO_NUM);
	
	who = inputMsg.m_source;		/* Who sent the message */
	callNum = inputMsg.m_type;		/* System call number */

	/* Process slot of caller. Misuse PM's own process slot if the kernel is
	 * calling. This can happen in case of synchronous alarms (CLOCK) or event
	 * like pending kernel signals (SYSTEM).
	 */
	currMp = &mprocTable[who < 0 ? PM_PROC_NR : who];
}

int main() {
/* Main routine of the process manager. */
	int result, s, pNum;
	MProc *rmp;
	sigset_t sigset;

	pmInit();		/* Initialize process manager tables. */

	while (true) {
		getWork();	/* Wait for an PM system call. */

		/* Check for system notifications first. Special cases. */
		if (callNum == SYN_ALARM) {
			pmExpireTimers(inputMsg.NOTIFY_TIMESTAMP);
			result = SUSPEND;		/* Don't reply */
		} else if (callNum == SYS_SIG) {	/* Signals pending */
			sigset = inputMsg.NOTIFY_ARG;
			if (sigismember(&sigset, SIGKSIG))
			  kernelSigPending();
			result = SUSPEND;		/* Don't reply */
		} 
		/* Else, if the system call number is valid, perform the call. */
		else if ((unsigned) callNum >= NCALLS) {
			result = ENOSYS;
		} else {
			result = callVec[callNum]();
		}

		/* Send the results back to the user to indicate completion. */
		if (result != SUSPEND) 
		  setReply(who, result);
	
		swapIn();	/* Maybe a process can be swapped in? */

		/* Send out all pending reply messages, including the answer to
		 * the call just made above. The processes must not be swapped out.
		 */
		for (pNum = 0, rmp = mprocTable; pNum < NR_PROCS; ++pNum, ++rmp) {
			/* In the meantime, the process may have been killed by a
			 * signal (e.g. if a lethal pending signal was unblocked)
			 * without the PM realizing it. If the slot is no longer in
			 * use or just a zombie, don't try to reply.
			 */
			if ((rmp->mp_flags & (REPLY | ONSWAP | IN_USE | ZOMBIE)) ==
					(REPLY | IN_USE)) {
				if ((s = send(pNum, &rmp->mp_reply)) != OK) {
					panic(__FILE__, "PM can't reply to", pNum);
				}
				rmp->mp_flags &= ~REPLY;
			}
		}
	}
	return OK;
}
