# u64_t add64u(u64_t i, unsigned j);
# u64_t add64ul(u64_t i, unsigned long j);
#
# Stack Layout:
#   esp + 16:  j
#   esp + 12:  high 32-bit of i
#   esp +  8:  low 32-bit of i
#   esp +  4:  resultPtr(u64_t *)
#	esp +  0:  retAddr

	.text
	.globl	add64u
	.type	add64u, @function
add64u:

	.globl	add64ul
	.type	add64ul, @function
add64ul:
	movl	4(%esp), %eax		# eax = result pointer
	movl	8(%esp), %edx		# edx = low 32-bit of i
	addl	16(%esp), %edx		# edx += j
	movl	%edx, (%eax)		# result[low 32-bit] = edx
	movl	12(%esp), %edx		# edx = high 32-bit of i
	adcl	$0, %edx			# edx += CF 
	movl	%edx, 4(%eax)		# result[high 32-bit] = edx
	retl

