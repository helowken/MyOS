# u64_t make64(unsigned long lo, unsigned long hi);
#
# Stack Layout:
#   esp + 12:  j
#   esp +  8:  i
#   esp +  4:  resultPtr(u64_t *)
#	esp +  0:  retAddr

	.text
	.globl	make64
	.type	make64, @function
make64:
	movl	4(%esp), %eax	# eax = resultPtr
	movl	8(%esp), %edx	# edx = i
	movl	%edx, (%eax)	# result[low 32-bit] = edx = i
	movl	12(%esp), %edx	# edx = j
	movl	%edx, 4(%eax)	# result[high 32-bit] = edx = j
	retl
