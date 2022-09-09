#include "kernel.h"
#include "signal.h"
#include "proc.h"

void handleException(unsigned exVector) {
/* An exception or unexpected interrupt has occurred. */
	typedef struct {
		char *msg;
		int sigNum;
		int minProcessor;
	} Exception;

	static Exception exData[] = {
		{ "Divide error", SIGFPE, 86 },
		{ "Debug exception", SIGTRAP, 86 },
		{ "Nonmaskale interrupt", SIGBUS, 86 },
		{ "Breakpoint", SIGEMT, 86 },
		{ "Overflow", SIGFPE, 86 },
		{ "Bounds check", SIGFPE, 186 },
		{ "Invalid opcode", SIGILL, 186 },
		{ "Coprocessor not available", SIGFPE, 186 },
		{ "Double fault", SIGBUS, 286 },
		{ "Copressor segment overrun", SIGSEGV, 286 },
		{ "Invalid TSS", SIGSEGV, 286 },
		{ "Segment not present", SIGSEGV, 286 },
		{ "Stack exception", SIGSEGV, 286 },	
		{ "General protection", SIGSEGV, 286 },
		{ "Page fault", SIGSEGV, 386 },		/* Not close */
		{ NIL_PTR, SIGILL, 0 },		/* Probably software trap */
		{ "Coprocessor error", SIGFPE, 386 }
	};
	register Exception *ep;
	Proc *savedProc;

	/* Save currProc, because it may be changed by debug statements. */
	savedProc = currProc;
	ep = &exData[exVector];

	if (exVector == 2) {	/* Spurious NMI on some machines */
		kprintf("Got spurious NMI\n");
		return;
	}
	
	/* If an exception occurs while running a process, the 
	 * 'kernelReenter' variable will be zero. Exceptions in interrupt 
	 * handlers or system traps will make 'kernelReenter' larger 
	 * than zero.
	 */
	if (kernelReenter == 0 && !isKernelProc(savedProc)) {
		kprintf("No kernel proc: %s\n", savedProc->p_name);
		//causeSig(procNum(savedProc), ep->sigNum);TODO
		return;
	}

	/* Exception in system code. This is not supposed to happen. */
	if (ep->msg == NIL_PTR || machine.processor < ep->minProcessor) 
	  kprintf("\nIntel-reserved exception %d\n", exVector);
	else
	  kprintf("\n%s\n", ep->msg);
	
	kprintf("kernelReenter = %d, ", kernelReenter);
	kprintf("process %d (%s), ", procNum(savedProc), savedProc->p_name);
	kprintf("pc = %u:0x%x", (unsigned) savedProc->p_reg.cs,
				(unsigned) savedProc->p_reg.pc);

	panic("exception in a kernel task", NO_NUM);
}
