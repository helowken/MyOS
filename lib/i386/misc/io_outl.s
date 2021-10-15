# Ouput one dword. 
#
# void outl(U16_t port, U32_t value);

	.text
	.globl	outl
	.type	outl, @function
outl:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx	# port
	movl	12(%ebp), %eax	# value
	outl	%dx				# output 1 dword
	popl	%ebp
	retl
