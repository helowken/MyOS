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
 *	if ((irqActiveIds[hook->irq] &= ~hook->id) == 0) 
 *	  outb(INT_CTLMASK, inb(INT_CTLMASK) & ~(1 << hook->irq));
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
	jb	.enableIrqAtSlave	
	movl	$INT2_CTL_MASK, %edx	/* Enable irq >=8 at the slave 8259 */ 
.enableIrqAtSlave:
	inb	%dx					/* inb(INT(2)_CTLMASK)a */
	andb	%ah, %al		/* al = MASK */
	outb	%dx				/* Clear bit at the 8259 */
.enableIrqDone:
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
.fillStart:
	movl	%ebx, (%esi)
	addl	$4, %esi
	decl	%eax
	jnz	.fillStart
	movl	16(%ebp), %eax		/* Any remaining bytes? */
	addl	$3, %eax			/* count %= 4 */
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






