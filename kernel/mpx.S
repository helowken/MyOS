#include "minix/config.h"
#include "minix/com.h"
#include "ibm/interrupt.h"
#include "const.h"
#include "protect.h"	
.include "sconst.inc"

#define TSS3_S_SP0	4

	.text
/* Set up a C stack frame on the monitor stack. */
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi

/* Copy the monitor GDT to the address space of kernel 
 * and switch over to it. */
	sgdt	gdt+GDT_SELECTOR			/* Get the monitor GDTR (GDT Register) */
	movl	gdt+GDT_SELECTOR+2, %esi	/* Absolute address of GDT */
	movl	$gdt, %ebx					/* Address of kernel GDT */
	movl	$(8*8), %ecx				/* Copy 8 descriptors */
.copyGDT:
	movb	%es:(%esi), %al
	movb	%al, (%ebx)
	incl	%esi
	incl	%ebx
	loop	.copyGDT
	
	movl	gdt+DS_SELECTOR+2, %eax		/* Base of kernel data */
	andl	$0xFFFFFF, %eax				/* Only 24 bits */
	addl	$gdt, %eax					/* Convert gdt address from virtual to physical (gdt + ds base) */
	movl	%eax, gdt+GDT_SELECTOR+2	/* Set base of GDT  */
	lgdt	gdt+GDT_SELECTOR			/* Switch over to kernel GDT */

/* Locate boot parameters, set up kernel segment registers and stack. */
	movl	8(%ebp), %ebx		/* Boot parameters offset (ss relative) */
	movl	12(%ebp), %edx		/* Boot parameters length */
	movl	16(%ebp), %eax		/* Address of image headers */
	movl	%eax, imgHdrPos				
	movw	%ds, %ax			/* Kernel data selector */
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss			/* ss = gs = fs = es = ds */
	movl	$kStackTop, %esp	/* Set sp to point to the top of kernel stack */

/* Call C startup code to set up a proper environment to run main(). */
	pushl	%edx
	pushl	%ebx
	pushl	$SS_SELECTOR
	pushl	$DS_SELECTOR
	pushl	$CS_SELECTOR
	calll	cstart				/* cstart(cs, ds, ss, paramOffset, paramSize) */
	addl	$(5*4), %esp

/* Reload GDTR, IDTR and the segment registers to global descriptor table 
 * set up by protectInit(). */
	lgdt	gdt+GDT_SELECTOR
	lidt	gdt+IDT_SELECTOR
	
	ljmp	$CS_SELECTOR, $.csInit
.csInit:
	movw	$DS_SELECTOR, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movw	$TSS_SELECTOR, %ax	/* No other TSS is used */
	ltr	%ax
	pushl	$0					/* Set flags to known good state */
	popfl						/* Clear nested task and interrupt enable */

	jmp	main					/* main() */

aa:
	jmp aa



/*=====================================================================*
 *                Interrupt Handlers                *
 *		Interrupt handlers for 386 32-bit protected mode		*
 *=====================================================================*/

/*=====================================================================*
 *                hwint00 - 07                *
 *=====================================================================*/
.macro hwintMaster irq
	calll	save			/* Save interrupted process state. */
	pushl	irqHandlers+\irq*4	/* irqHandlers[irq] */
	calll	handleInterrupt		/* handleInterrupt(irqHandlers[irq]) */
	popl	%ecx
	cmpl	$0, irqActiveIds+\irq*4	/* Is interrupt still active? */
	jz	0f
	inb	$INT_CTL_MASK		/* Get current mask */
	orb	1<<\irq, %al		/* Mask irq */
	outb	$INT_CTL_MASK	/* Disable the irq */
0:
	movb	$END_OF_INT, %al	
	outb	$INT_CTL		/* Reenable master 8259 */
	retl					/* Restart (another) process. (retAddr has been pushed by save().) */
.endm

/* Each of these entry points is an expansion of the hwintMaster macro. */

/* void hwint00();	
 * Intrrupt routine for irq 0 (the clock). */
	.globl	hwint00
	.type	hwint00, @function
	.align	16
hwint00:
	hwintMaster(0)
		
/* void hwint01();
 * Interrupt routine for irq 1 (keyboard). */
	.globl	hwint01
	.type	hwint01, @function
	.align	16
hwint01:
	hwintMaster(1)

/* void hwint02();
 * Interrupt routine for irq 2 (cascade!). */
	.globl	hwint02
	.type	hwint02, @function
	.align	16
hwint02:
	hwintMaster(2)

/* void hwint03();
 * Interrupt routine for irq 3 (second serial). */
	.globl	hwint03
	.type	hwint03, @function
	.align	16
hwint03:
	hwintMaster(3)

/* void hwint04();
 * Interrupt routine for irq 4 (first serial). */
	.globl	hwint04
	.type	hwint04, @function
	.align	16
hwint04:
	hwintMaster(4)

/* void hwint05();
 * Interrupt routine for irq 5 (XT winchester). */
	.globl	hwint05
	.type	hwint05, @function
	.align	16
hwint05:
	hwintMaster(5)

/* void hwint06();
 * Interrupt routine for irq 6 (floppy). */
	.globl	hwint06
	.type	hwint06, @function
	.align	16
hwint06:
	hwintMaster(6)

/* void hwint07();
 * Interrupt routine for irq 7 (printer). */
	.globl	hwint07
	.type	hwint07, @function
	.align	16
hwint07:
	hwintMaster(7)

/*=====================================================================*
 *                hwint08 - 15                *
 *=====================================================================*/
.macro hwintSlave irq
	retl
.endm

/* Each of these entry points is an expansion of the hwintSlave macro. */

/* void hwint08();
 * Interrupt routine for irq 8 (realtime clock). */
	.globl	hwint08
	.type	hwint08, @function
	.align	16
hwint08:
	hwintSlave(8)

/* void hwint09();
 * Interrupt routine for irq 9 (irq 2 redirected). */
	.globl	hwint09
	.type	hwint09, @function
	.align	16
hwint09:
	hwintSlave(9)

/* void hwint10();
 * Interrupt routine for irq 10. */
	.globl	hwint10
	.type	hwint10, @function
	.align	16
hwint10:
	hwintSlave(10)

/* void hwint11();
 * Interrupt routine for irq 11. */
	.globl	hwint11
	.type	hwint11, @function
	.align	16
hwint11:
	hwintSlave(11)

/* void hwint12();
 * Interrupt routine for irq 12. */
	.globl	hwint12
	.type	hwint12, @function
	.align	16
hwint12:
	hwintSlave(12)

/* void hwint13();
 * Interrupt routine for irq 13 (FPU exception). */
	.globl	hwint13
	.type	hwint13, @function
	.align	16
hwint13:
	hwintSlave(13)

/* void hwint14();
 * Interrupt routine for irq 14 (AT winchester). */
	.globl	hwint14
	.type	hwint14, @function
	.align	16
hwint14:
	hwintSlave(14)

/* void hwint15();
 * Interrupt routine for irq 15. */
	.globl	hwint15
	.type	hwint15, @function
	.align	16
hwint15:
	hwintSlave(15)


/*=====================================================================*
 *                Exception Handlers                *
 *=====================================================================*/
	.globl	divideError
	.type	divideError, @function
divideError:
	pushl	DIVIDE_VECTOR
	jmp	exception


	.globl	singleStepException
	.type	singleStepException, @function
singleStepException:
	pushl	DEBUG_VECTOR
	jmp exception


	.globl	nmi
	.type	nmi, @function
nmi:
	pushl	NMI_VECTOR
	jmp exception


	.globl	breakpointException
	.type	breakpointException, @function
breakpointException:
	pushl	BREAKPOINT_VECTOR
	jmp	exception


	.globl	overflow
	.type	overflow, @function
overflow:
	pushl	OVERFLOW_VECTOR
	jmp	exception


	.globl	boundsCheck
	.type	boundsCheck, @function
boundsCheck:
	pushl	BOUNDS_VECTOR
	jmp	exception


	.globl	invalOpcode
	.type	invalOpcode, @function
invalOpcode:
	pushl	INVAL_OP_VECTOR
	jmp exception


	.globl	coprNotAvailable
	.type	coprNotAvailable, @function
coprNotAvailable:
	pushl	COPROC_NOT_VECTOR
	jmp	exception


	.globl	doubleFault
	.type	doubleFault, @function
doubleFault:
	pushl	DOUBLE_FAULT_VECTOR
	jmp	errException


	.globl	coprSegOverrun
	.type	coprSegOverrun, @function
coprSegOverrun:
	pushl	COPROC_SEG_VECTOR
	jmp exception


	.globl	invalTss
	.type	invalTss, @function
invalTss:
	pushl	INVAL_TSS_VECTOR
	jmp	errException


	.globl	segmentNotPresent
	.type	segmentNotPresent, @function
segmentNotPresent:
	pushl	SEG_NOT_VECTOR
	jmp errException


	.globl	stackException
	.type	stackException, @function
stackException:
	pushl	STACK_FAULT_VECTOR
	jmp errException


	.globl	generalProtection
	.type	generalProtection, @function
generalProtection:
	pushl	PROTECTION_VECTOR
	jmp errException


	.globl	pageFault
	.type	pageFault, @function
pageFault:
	pushl	PAGE_FAULT_VECTOR
	jmp errException


	.globl	coprError
	.type	coprError, @function
coprError:
	pushl	COPROC_ERR_VECTOR
	jmp	exception


/*=====================================================================*
 *                exception                *
 *=====================================================================*/
	.type	exception, @function
	.align	16
exception:
	movl	$0, %ss:(errorCode)		/* Clear errorCode */
	popl	%ss:(exVector)
	jmp	.commonException	
	

/*=====================================================================*
 *                errException                *
 *=====================================================================*/
	.type	errException, @function
	.align	16
errException:
	popl	%ss:(errorCode)			
	popl	%ss:(exVector)
.commonException:				/* Common for all exceptions */
	pushl	%eax
	movl	4(%esp), %eax		/* Save old EIP */
	movl	%eax, %ss:(oldEip)
	movl	8(%esp), %eax		/* Save old CS */
	movl	%eax, %ss:(oldCs)
	movl	12(%esp), %eax		/* Save old EFLAGS */
	movl	%eax, %ss:(oldEflags)
	popl	%eax
	calll	save
	pushl	oldEflags
	pushl	oldCs
	pushl	oldEip
	pushl	errorCode
	pushl	exVector
	calll	handleException		/* handleException(exVector, errorCode, oldEip, oldCs, oldEflags); */
	addl	$(5*4), %esp
	retl


/*=====================================================================*
 *                save                *
 *=====================================================================*/

/* Save for protected mode. 
   The stack already points into the process table, or has already been
   switched to the kernel stack. 

   If the stack points into Proc.StackFrame[P_STACKTOP], because of stack-switch,
   the processor has pushed EIP, CS, EFLAGS, ESP, SS on the stack.
 */

	.type	save, @function
	.align	16
save:
	cld									
	pushal						/* Save all general registers */
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs					/* Save ds, es, fs, gs */
	movw	%ss, %dx			/* ss is kernel data segment (See above) */
	movw	%dx, %ds			/* Load rest of kernel segments */
	movw	%dx, %es			/* Kernel does not use fs, gs */
	movl	%esp, %eax			/* Prepare to return */
	incb	kernelReentryCount	/* From -1 if not reentering */
	jnz	.setRestart				/* Stack is already kernel stack */
	movl	$kStackTop, %esp	
	pushl	restart				/* Build return address for interrupt handler */
	xorl	%ebp, %ebp			/* For stacktrace */
	jmp	*RETADDR-P_STACKBASE(%eax)	/* jmp *%eax is AT&T syntax for jmp eax. */

	.align	4
.setRestart:
	pushl	.doRestart
	jmp	*RETADDR-P_STACKBASE(%eax)

/*=====================================================================*
 *                s_call                *
 *=====================================================================*/ 

/* C calling conventions:
 * A procedure must preserve the values of the EBX, ESP, EBP, ESI and EDI.
 */

	.globl	s_call
	.type	s_call, @function
	.align	16
s_call:
	cld							/* Set direction flag to a known value */
	subl	$(6*4), %esp		/* Skip RETADDR, eax, ecx, edx, ebx, temp */
	pushl	%ebp				/* Stack already points into procTable */
	pushl	%esi
	pushl	%edi
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movw	%ss, %dx
	movw	%dx, %ds
	movw	%dx, %es
	incb	kernelReentryCount
	movl	%esp, %esi			/* Assumes P_STACKBASE == 0 */
	movl	$kStackTop, %esp
	xorl	%ebp, %ebp			/* End of inline save() */
	
	pushl	%ebx				/* Pointer to user message */
	pushl	%eax				/* Source / Destination */
	pushl	%ecx				/* SEND / RECEIVE / BOTH */
	calll	sys_call			/* sys_call(function, srcDest, msgPtr) */
	movl	%eax, AXREG(%esi)	/* sys_call must preserve esi. (C calling conventions) */

/* Fall into code to restart proc/task running. */

/*=====================================================================*
 *                restart                *
 *=====================================================================*/ 

/* Restart the current process or the next process if it is set. */ 
	.globl	restart 
	.type	restart, @function
restart:
	cmpl	$0, nextProc		/* See if another process is scheduled. */
	jz	.loadProc
	movl	nextProc, %eax
	movl	%eax, currProc		/* Schedule new process */
	movl	$0, nextProc
.loadProc:
	movl	currProc, %esp		/* esp = Proc.StackFrame */
	lldt	P_LDT_SEL(%esp)		/* Enable process' segment descriptors */
	leal	P_STACKTOP(%esp), %eax	/* Arrange for next interrupt */
	movl	%eax, tss+TSS3_S_SP0	/* To save state in process table */
.doRestart:
	decb	kernelReentryCount 
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popal
	addl	$4, %esp			/* Skip return address */
	iretl						/* Continue process */

/*=====================================================================*
 *                level0_call                *
 *=====================================================================*/ 
	.globl	level0_call
	.type	level0_call, @function
level0_call:
	calll save
	jmp level0Func



	.section	.bss
	.lcomm	kStack, K_STACK_BYTES	/* Kernel stack */
	.lcomm  kStackTop, 0		/* Top of kernel stack, just a position tag, no size. */
	.lcomm	exVector, 4	
	.lcomm	errorCode, 4
	.lcomm	oldEip, 4
	.lcomm	oldCs, 4
	.lcomm	oldEflags, 4


