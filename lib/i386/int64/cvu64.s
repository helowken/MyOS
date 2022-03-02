# u64_t cvu64(unsigned i);
# u64_t cvul64(unsigned long i);
#
# Stack Layout:
#   esp +  8: i 
#   esp +  4: resultPtr(u64_t *)
#	esp +  0: retAddr

	.text
	.globl	cvu64
	.type	cvu64, @function
cvu64:
	
	.globl	cvul64
	.type	cvul64, @function
cvul64:	
	movl	4(%esp), %eax		# eax = result pointer
	movl	8(%esp), %edx		# edx = i
	movl	%edx, (%eax)		# result[low 32-bit] = i
	movl	$0, 4(%eax)			# result[high 32-bit] = 0
	retl



