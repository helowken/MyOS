	.text
	.globl	_sigReturn
	.type	_sigReturn, @function
_sigReturn:
	addl	$16, %esp
	jmp	sigReturn
