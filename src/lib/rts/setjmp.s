	.text
	.globl	__setjmp
	.type	__setjmp, @function
__setjmp:


# TODO==========
	xorl	%eax, %eax
	retl
# TODO==========

	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx
	movl	12(%ebp), %ecx
	movl	%ecx, 16(%edx)
	cmpl	$0x0, 12(%ebp)
	je	.I1_1
	movl	8(%ebp), %edx
# Kill edx
	addl	$12, %edx
	pushl	%edx
	calll	__newSigset
	popl	%ecx
.I1_1:
	movl	8(%ebp), %ebx
	pushl	%ebp
	leal	8(%ebp), %eax
	pushl	%eax
	pushl	16(%ebp)
	movl	$12, %ecx
	calll	.sti
	xorl	%eax, %eax
	leave
	retl


	.type	_fill_ret_area, @function
_fill_ret_area:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	leave
	retl


	.globl	longjmp
	.type	longjmp, @function
longjmp:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx
	cmpl	$0, 16(%edx)
	je	.I3_2
# Kill %edx
	addl	$12, %edx
	pushl	%edx
	calll	__oldSigset
	popl	%ecx
.I3_2:
	pushl	8(%ebp)
	pushl	_gtobuf
	movl	$3, %ecx
	calll	.blm
	movl	12(%ebp), %edx
	pushl	%edx
	testl	%edx, %edx
	jne	.I3_3
	popl	%edx
	incl	%edx
	pushl	%edx
.I3_3:
	calll	_fill_ret_area
	popl	%ecx
	movl	_gtobuf, %ebx
	jmpl	*.gto



	.section	.bss
	.lcomm	_gtobuf, 12



