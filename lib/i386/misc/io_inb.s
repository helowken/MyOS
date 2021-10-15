# Input one byte.
#
# unsigned inb(U16_t port);

	.text
	.globl	inb
	.type	inb, @function
inb:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx	# port
	xorl	%eax, %eax
	inb	%dx					# read 1 byte
	popl	%ebp
	retl
