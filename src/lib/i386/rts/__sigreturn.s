	.text
	.globl	_sigreturn
	.type	_sigreturn, @function
_sigreturn:
	addl	$16, %esp
	jmp	sigreturn
