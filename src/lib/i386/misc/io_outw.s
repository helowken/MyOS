# Ouput one word. 
#
# void outw(u16_t port, u16_t value);

	.text
	.globl	outw
	.type	outw, @function
outw:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx	# port
	movl	12(%ebp), %eax	# value
	outw	%dx				# output 1 word
	popl	%ebp
	retl
