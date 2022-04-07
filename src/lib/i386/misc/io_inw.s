# Input one word. 
#
# unsigned inw(u16_t port);

	.text
	.globl	inw
	.type	inw, @function
inw:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx	# port
	xorl	%eax, %eax
	inw	%dx					# read 1 word
	popl	%ebp
	retl
