	.text
	.globl	.gto
	.type	.gto, @function
.gto:
	movl	8(%ebx), %ebp
	movl	4(%ebx), %esp
	jmp	*(%ebx)
