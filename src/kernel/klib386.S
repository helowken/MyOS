#include "minix/config.h"
#include "minix/const.h"
#include "const.h"
#include "protect.h"
.include "sconst.inc"

/* This file contains a number of assembly code utility routines needed by the kernel. */

	.text
/*=====================================================================*
 *                idleTask                *
 *=====================================================================*/

/* void idleTask();
 * This task is called when the system has nothing else to do. The HLT
 * introduction puts the processor in a state where it draws minimum power.
 */

	.globl	idleTask
	.type	idleTask, @function
idleTask:
	push $halt
	calll	level0		/* level0(halt) */
	popl %eax
	jmp idleTask

/* hlt instruction is a privileged instruction, so we need to call it by a gate.
 * It stops instruction execution and places the processor in a HALT state.
 * An enabled interrupt will resume execution.
 */
halt:
	sti
	hlt
	cli
	retl

/*=====================================================================*
 *                level0                *
 *=====================================================================*/

/* void level0(void (*func)(void));
 * Call a function at permission level 0. This allows kernel tasks to do
 * things that are only possible at the most privileged CPU level.
 */

	.globl	level0
	.type	level0, @function
level0:
	movl	4(%esp), %eax
	movl	%eax, level0Func
	int $LEVEL0_VECTOR
	retl

/*=====================================================================*
 *                physCopy                *
 *=====================================================================*/
 
/* void physCopy(phys_bytes source, phys_bytes dest, phys_bytes count);
 *
 * Copy a block of physical memory. 
 */

.equ PC_ARGS,	4 + 4 + 4 + 4 + 4	/* ds | es | edi | esi | retAddr */

	.globl	physCopy
	.type	physCopy, @function
	.align	16
physCopy:
	cld						/* Set esi and edi to be incremented. */
	pushl	%esi
	pushl	%edi
	pushl	%es
	pushl	%ds

	movl	$FLAT_ES_SELECTOR, %eax
	movw	%ax, %es
	movw	%ax, %ds		/* ds = es = flat 4 GB (less privileged) */

	movl	PC_ARGS(%esp), %esi			/* source */
	movl	PC_ARGS+4(%esp), %edi		/* dest */
	movl	PC_ARGS+4+4(%esp), %eax		/* count */

	cmpl	$10, %eax		/* Avoid align overhead for small counts */
	jb	.pcSmall
	movl	%esi, %ecx		/* Align source for dwords copy */
	negl	%ecx
	andl	$3, %ecx
	subl	%ecx, %eax
	rep
	movsb	

	movl	%eax, %ecx
	shrl	$2, %ecx		/* Count of dwords */
	rep
	movsd	

	andl	$3, %eax
.pcSmall:
	xchgl	%eax, %ecx		/* Remainder */
	rep
	movsb	

	popl	%ds
	popl	%es
	popl	%edi
	popl	%esi
	retl

/*=====================================================================*
 *                enableIrq                *
 *=====================================================================*/

/* void enableIrq(IrqHook *hook);
 * Enable an interrupt request line by clearing an 8259 bit.
 *
 * Equivalent C code:
 *
 *	if ((irqActiveIds[hook->irq] &= ~hook->id) != 0) 
 *    return;
 *  v = ~(1 << (hook->irq % 8));
 *  port = (hook->irq < 8) ? INT_CTL_MASK : INT2_CTL_MASK;
 *  mask = inb(port);
 *	outb(port, mask & v);
 */
	
	.globl	enableIrq
	.type	enableIrq, @function
	.align	16
enableIrq:
	pushl	%ebp	
	movl	%esp, %ebp
	pushfl
	cli
	movl	8(%ebp), %eax	/* hook */
	movl	8(%eax), %ecx	/* hook->irq */
	movl	12(%eax), %eax	/* hook->id */
	notl	%eax			/* ~hook->id */
	andl	%eax, irqActiveIds(,%ecx,4)	/* Clear this id bit */
	jnz	.enableIrqDone		/* if (... != 0) still masked by other handlers, go to end. */	
	movb	$~1, %ah		/* ~(1 << irq) => (~1) rotate irq */
	rolb	%cl, %ah		/* ah = ~(1 << (irq % 8)) */
	movl	$INT_CTL_MASK, %edx	/* Enable irq < 8 at the master 8259 */
	cmpb	$8, %cl			
	jb	0f
	movl	$INT2_CTL_MASK, %edx	/* Enable irq >=8 at the slave 8259 */ 
0:
	inb	%dx					/* inb(INT(2)_CTL_MASK) */
	andb	%ah, %al		/* al = MASK */
	outb	%dx				/* Clear bit at the 8259 */
.enableIrqDone:
	popfl
	leave
	retl

/*=====================================================================*
 *                disableIrq                *
 *=====================================================================*/

/* void disableIrq(IrqHook *hook);
 *
 * Disable an interrupt request line by setting an 8259 bit.
 * Equivalent C code for:
 *
 *	irqActiveId[hook->irq] |= hook->id;
 *  v = 1 << (hook->irq % 8);
 *  port = (hook->irq < 8) ? INT_CTL_MASK : INT2_CTL_MASK;
 *  mask = inb(port);
 *  if (mask & v)
 *    return 0;
 *  outb(port, mask | v);
 *  return 1;
 */
	.globl	disableIrq
	.type	disableIrq, @function
	.align	16
disableIrq:
	pushl	%ebp
	movl	%esp, %ebp
	pushfl
	cli
	movl	8(%ebp), %eax		/* hook */
	movl	8(%eax), %ecx		/* hook->irq */
	movl	12(%eax), %eax		/* hook->id */
	orl	%eax, irqActiveIds(,%ecx,4)		/* Set this id bit */
	movb	$1, %ah
	rolb	%cl, %ah			/* ah = 1 << (irq % 8) */
	movl	$INT_CTL_MASK, %edx		/* Disable irq < 8 at the master 8259 */
	cmpb	8, %cl
	jb	0f
	movl	$INT2_CTL_MASK, %edx	/* Disable irq >= 8 at the slave 8259 */
0:	
	inb	%dx						/* inb(INT(2)_CTL_MASK) */
	testb	%ah, %al
	jnz .irqDisabledAlready		/* Already disabled? */
	orb	%ah, %al
	outb	%dx					/* Set bit at the 8259 */
	movl	$1, %eax			/* Return true since disabled by this function */
	popfl
	leave
	retl
.irqDisabledAlready:
	xorl	%eax, %eax			/* Return false since already disabled */
	popfl
	leave
	retl

/*=====================================================================*
 *                copyMessage                *
 *=====================================================================*/

/* void copyMessage(int src, phys_clicks srcAddr, vir_bytes srcOffset, 
 *			phys_clicks dstAddr, vir_bytes dstOffset);
 *
 * This routine makes a fast copy of a message from anywhere in the address
 * space to anywhere else. It also copies the source address provided as a
 * parameter to the call into the first word of the destination message.
 *
 * Note that the message size, "Msize" is in DWORDS (not bytes) and must be
 * set correctly. Changing the definition of message in the type file and 
 * not changing it here will lead to total disaster.
 */

.equ CM_ARGS, 4 + 4 + 4 + 4 + 4		 /* 4	+  4	+  4	+  4	+  4	*/
/*			 es  ds  edi esi retAddr   src srcAddr srcOffset dstAddr dstOffset */

	.globl	copyMessage
	.type	copyMessage, @function
copyMessage:
	cld
	pushl	%esi
	pushl	%edi
	pushl	%ds
	pushl	%es
	
	movl	$FLAT_ES_SELECTOR, %eax
	movw	%ax, %ds
	movw	%ax, %es

	movl	CM_ARGS+4(%esp), %esi		/* src addr */
	shll	$CLICK_SHIFT, %esi 
	addl	CM_ARGS+4+4(%esp), %esi		/* + src offset */
	movl	CM_ARGS+4+4+4(%esp), %edi	/* dst addr */
	shll	$CLICK_SHIFT, %edi
	addl	CM_ARGS+4+4+4+4(%esp), %edi		/* + dst offset */
	
	movl	CM_ARGS(%esp), %eax			/* src (process number of sender) */
	stosl								/* Copy number of sender to dst message. */
	addl	$4, %esi					/* Do not copy first dword */
	movl	$(Msize - 1), %ecx			/* Remember, first dword does not count. */
	rep
	movsd								/* Copy the message */

	popl	%es
	popl	%ds
	popl	%edi
	popl	%esi
	retl								

/*=====================================================================*
 *                physMemset                *
 *=====================================================================*/

/* void physMemset(phys_bytes source, unsigned long pattern, phys_bytes count);
 *
 * Fill a block of physical memory iwth pattern.
 */
	.globl	physMemset
	.type	physMemset, @function
	.align	16
physMemset:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%ebx
	pushl	%ds
	movl	8(%ebp), %esi		/* source */
	movl	16(%ebp), %eax		/* count */
	movl	$FLAT_ES_SELECTOR, %ebx		
	movw	%bx, %ds
	movl	12(%ebp), %ebx		/* pattern */
	shrl	$2, %eax			/* count /= 4, becuase pattern is unsigned long. */
	jz	.beforeRemainFill
.fillStart:
	movl	%ebx, (%esi)
	addl	$4, %esi
	decl	%eax
	jnz	.fillStart
.beforeRemainFill:
	movl	16(%ebp), %eax		/* Any remaining bytes? */
	andl	$3, %eax			/* count %= 4 */
.remainFill:
	cmpl	$0, %eax
	jz	.fillDone
	movb	12(%ebp), %bl		/* pattern[ebp], ebp may be 0, 1, 2. */
	movb	%bl, (%esi)
	addl	$1, %esi
	incl	%ebp
	decl	%eax
	jmp	.remainFill
.fillDone:
	popl	%ds
	popl	%ebx
	popl	%esi
	popl	%ebp
	retl

/*=====================================================================*
 *                readTsc                *
 *=====================================================================*/

/* void readTsc(unsigned long *high, unsigned long *low);
 *
 * Read the cycle counter of the CPU. Pentium and up.
 */
	.globl	readTsc
	.type	readTsc, @function
	.align	16
readTsc:
	rdtsc
	pushl	%ebp
	movl	8(%esp), %ebp		
	movl	%edx, (%ebp)		/* High 32-bit */
	movl	12(%esp), %ebp		
	movl	%eax, (%ebp)		/* Low 32-bit */
	popl	%ebp
	retl

/*=====================================================================*
 *                physInsb                *
 *=====================================================================*/

/* void physInsb(u16_t port, phys_bytes buf, size_t count);
 * 
 * Input an array from an I/O port. Absolute address version of insb().
 */
	.globl	physInsb
	.type	physInsb, @function
	.align	16
physInsb:
	pushl	%ebp
	movl	%esp, %ebp
	cld
	pushl	%edi
	pushl	%es
	movl	$FLAT_ES_SELECTOR, %ecx
	movw	%cx, %es
	movl	8(%ebp), %edx		/* Port to read from */
	movl	12(%ebp), %edi		/* Destination addr */
	movl	16(%ebp), %ecx		/* byte count */
	rep	insb					/* Input many bytes */
	popl	%es
	popl	%edi
	popl	%ebp
	retl

/*=====================================================================*
 *                physInsw                *
 *=====================================================================*/

/* void physInsw(u16_t port, phys_bytes buf, size_t count);
 *
 * Input an array from an I/O port. Absolute address version of insw().
 */
	.globl	physInsw
	.type	physInsw, @function
	.align	16
physInsw:
	pushl	%ebp
	movl	%esp, %ebp
	cld
	pushl	%edi
	pushl	%es
	movl	$FLAT_ES_SELECTOR, %ecx
	movw	%cx, %es
	movl	8(%ebp), %edx		/* Port to read from */
	movl	12(%ebp), %edi		/* Destination addr */
	movl	16(%ebp), %ecx		/* byte count */
	shrl	$1,	%ecx			/* Word count */
	rep	insw					/* Input many bytes */
	popl	%es
	popl	%edi
	popl	%ebp
	retl

/*=====================================================================*
 *                physOutsb                *
 *=====================================================================*/

/* void physOutsb(u16_t port, phys_bytes buf, size_t count);
 *
 * Output an array to an I/O port. Absolute address version of outsb().
 */
	.globl	physOutsb
	.type	physOutsb, @function
physOutsb:
	pushl	%ebp
	movl	%esp, %ebp
	cld
	pushl	%esi
	pushl	%ds
	movl	$FLAT_ES_SELECTOR, %ecx
	movw	%cx, %ds
	movl	8(%ebp), %edx		/* Port to write to */
	movl	12(%ebp), %esi		/* Source addr */
	movl	16(%ebp), %ecx		/* Byte count */
	rep	outsb					/* Output many bytes */
	popl	%ds
	popl	%esi
	popl	%ebp
	retl

/*=====================================================================*
 *                physOutsw                *
 *=====================================================================*/

/* void physOutsw(u16_t port, phys_bytes buf, size_t count);
 *
 * Output an array to an I/O port. Absolute address version of outsw().
 */
	.globl	physOutsw
	.type	physOutsw, @function
physOutsw:
	pushl	%ebp
	movl	%esp, %ebp
	cld
	pushl	%esi
	pushl	%ds
	movl	$FLAT_ES_SELECTOR, %ecx
	movw	%cx, %ds
	movl	8(%ebp), %edx		/* Port to write to */
	movl	12(%ebp), %esi		/* Source addr */
	movl	16(%ebp), %ecx		/* Byte count */
	shrl	$1, %ecx			/* Word count */
	rep	outsw					/* Output many bytes */
	popl	%ds
	popl	%esi
	popl	%ebp
	retl

/*=====================================================================*
 *                monitor                *
 *=====================================================================*/

/* void monitor();
 * 
 * Return to monitor.
 */
	.globl	monitor
	.type	monitor, @function
monitor:
	movl	monSp, %esp			/* Restore monitor stack pointer */
	movw	$SS_SELECTOR, %dx	/* Monitor data segment */
	movw	%dx, %ds
	movw	%dx, %es
	movw	%dx, %fs
	movw	%dx, %gs
	movw	%dx, %ss
	popl	%edi			/* Restore ebp, esi, edi (see the top of mpx.S) */
	popl	%esi
	popl	%ebp
	retfw					/* Return to the monitor, cs and ip are 16-bits.
							   (see "push MCS_SELECTOR and ret386" in boothead.s) */
					
/*=====================================================================*
 *                reset                *
 *=====================================================================*/

/* void reset();
 *
 * Reset the system by loading IDT with offset 0 and interrupting.
 */
	.globl	reset
	.type	reset, @function
reset:
	lidt	idtZero
	int	$3					/* Anything goes, the 386 will not like it */

	
	.section	.data
	.type	idtZero, @object
	.align	8
idtZero:
	.zero	8







