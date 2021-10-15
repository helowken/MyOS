# Input one dword.
#
# unsigned inl(U16_t port);

	.text
	.globl	inl
	.type	inl, @function
inl:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx	# port
	xorl	%eax, %eax
	inl	%dx					# read 1 dword
	popl	%ebp
	retl
