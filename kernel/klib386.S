#include "minix/config.h"
#include "minix/const.h"
#include "const.h"
#include "protect.h"
.include "sconst.inc"

/* This file contains a number of assembly code utility routines needed by the kernel. */

	.text

/*=====================================================================*
 *                physCopy                *
 *=====================================================================*
 * void physCopy(phys_bytes source, phys_bytes dest, phys_bytes count);
 * Copy a block of physical memory. 
 */
.equ PC_ARGS,	2 + 2 + 4 + 4 + 4	/* ds | es | edi | esi | retAddr */

	.globl	physCopy
	.type	physCopy, @function
	.align	16
physCopy:
	cld						/* Set esi and edi to be incremented. */
	pushl	%esi
	pushl	%edi
	pushw	%es
	pushw	%ds

	movl	$FLAT_ES_SELECTOR, %eax
	movw	%ax, %es
	movw	%ax, %ds		/* ds = es = flat 4 GB (less privileged) */

	movl	PC_ARGS(%esp), %esi		/* source */
	movl	PC_ARGS+4(%esp), %edi	/* dest */
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

	popw	%ds
	popw	%es
	popl	%edi
	popl	%esi
	retl

/*=====================================================================*
 *                enableIrq                *
 *=====================================================================*
 * void enableIrq(IrqHook *hook);
 * Enable an interrupt request line by clearing an 8259 bit.
 * Equivalent C code:
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



