# Ouput one byte. 
#
# void outb(U16_t port, U8_t value);

	.text
	.globl	outb
	.type	outb, @function
outb:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx	# port
	movl	12(%ebp), %eax	# value
	outb	%dx				# output 1 byte
	popl	%ebp
	retl
