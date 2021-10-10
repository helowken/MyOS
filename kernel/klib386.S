#include "minix/config.h"
#include "minix/const.h"
#include "const.h"
#include "protect.h"
.include "sconst.inc"

# This file contains a number of assembly code utility routines needed by the kernel.

	.text

#*=====================================================================*
#*                physCopy                *
#*=====================================================================*
# void physCopy(phys_bytes source, phys_bytes dest, phys_bytes count);
### Copy a block of physical memory.
.equ	PC_ARGS,	2 + 2 + 4 + 4 + 4	# ds | es | edi | esi | retAddr 

	.globl	physCopy
	.type	physCopy, @function
	.align	16
physCopy:
	cld									# Set esi and edi to be incremented.
	pushl	%esi
	pushl	%edi
	pushw	%es
	pushw	%ds

	movl	$FLAT_ES_SELECTOR, %eax
	movw	%ax, %es
	movw	%ax, %ds					# ds = es = flat 4 GB (less privileged)

	movl	PC_ARGS(%esp), %esi			# source
	movl	PC_ARGS+4(%esp), %edi		# dest
	movl	PC_ARGS+4+4(%esp), %eax		# count

	cmpl	$10, %eax					# Avoid align overhead for small counts
	jb	.pcSmall
	movl	%esi, %ecx					# Align source for dwords copy
	negl	%ecx
	andl	$3, %ecx
	subl	%ecx, %eax
	rep
	movsb	

	movl	%eax, %ecx
	shrl	$2, %ecx					# Count of dwords
	rep
	movsd	

	andl	$3, %eax
.pcSmall:
	xchgl	%eax, %ecx					# Remainder 
	rep
	movsb	

	popw	%ds
	popw	%es
	popl	%edi
	popl	%esi
	retl


