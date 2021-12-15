#ifndef SYSTEM_H
#define SYSTEM_H

#include "kernel.h"
#include "proto.h"
#include "proc.h"

int doUnused(Message *msg);

/*=====================================================================*
 *                 Process management.               *
 *=====================================================================*/ 

/* A process forked a new process. */
int doFork(Message *msg);

/* Update process after execute. */
int doExec(Message *msg);

/* Clean up after process exit. */
int doExit(Message *msg);

/* Set scheduling priority. */
int doNice(Message *msg);

/* System privileges control. */
int doPrivCtl(Message *msg);

/* Request a trace operation. */
int doTrace(Message *msg);


/*=====================================================================*
 *                  Signal handling.              *
 *=====================================================================*/ 

/* Cause a process to be signaled. */
int doKill(Message *msg);

/* PM checks for pending signals. */
int doGetKernelSig(Message *msg);

/* PM finished processing signal. */
int doEndKernelSig(Message *msg);

/* Start POSIX-style signal. */
int doSigSend(Message *msg);

/* Return from POSIX-style signal. */
int doSigReturn(Message *msg);


/*=====================================================================*
 *                   Device I/O.             *
 *=====================================================================*/ 

/* Interrupt control operations. */
int doIrqCtl(Message *msg);

/* inb, inw, inl, outb, outw, outl */
int doDevIO(Message *msg);

/* insb, insw, outsb, outsw */
int doStrDevIO(Message *msg);

/* Vector with devio requests. */
int doVecDevIO(Message *msg);

/* Real-mode BIOS calls. */
int doInt86(Message *msg);


/*=====================================================================*
 *                   Memory management.             *
 *=====================================================================*/ 

/* Set up a process memory map. */
int doNewMap(Message *msg);

/* Add segment and get selector. */
int doSegCtl(Message *msg);

/* Write char to memory area. */
int doMemset(Message *msg);


/*=====================================================================*
 *                   Copying.             *
 *=====================================================================*/ 

/* Map virtual to physcal address. */
int doUmap(Message *msg);

/* Use pure virtual addressing. */
int doVirCopy(Message *msg);

/* Use physical addressing. */
int doPhysCopy(Message *msg);

/* Vector with copy requests. */
int doVirVecCopy(Message *msg);

/* Vector with copy requests. */
int doPhysVecCopy(Message *msg);


/*=====================================================================*
 *                   Clock functionality.             *
 *=====================================================================*/ 

/* Get uptime and process times. */
int doTimes(Message *msg);

/* Schedule a synchronous alarm. */
int doSetAlarm(Message *msg);


/*=====================================================================*
 *                   System control.             *
 *=====================================================================*/ 

/* Abort MINIX. */
int doAbort(Message *msg);

/* Request system information. */
int doGetInfo(Message *msg);

#endif
