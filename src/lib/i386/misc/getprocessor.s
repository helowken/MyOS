	.text
	.global	getProcessor
	.type	getProcessor, @function
getProcessor:
	pushl	%ebp
	movl	%esp, %ebp
	movl	$586, %eax
	leave
	retl

