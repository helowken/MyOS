# u64_t mul64u(unsigned long i, unsigned j);
#
# Stack Layout:
#   esp + 12:  j
#   esp +  8:  i
#   esp +  4:  resultPtr(u64_t *)
#	esp +  0:  retAddr
#
# Before this call, eax = resultPtr. (See the assembly of c code)

	.text
	.globl	mul64u
	.type	mul64u, @function
mul64u:
	movl	4(%esp), %ecx		# ecx = result pointer 
	movl	8(%esp), %eax		# eax = i
	mull	12(%esp)			# edx:eax = i * j
	movl	%eax, (%ecx)		# result[low 32-bit] = eax
	movl	%edx, 4(%ecx)		# result[high 32-bit] = edx
	movl	%ecx, %eax			# eax = result pointer
	retl


