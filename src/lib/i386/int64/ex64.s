# unsigned long ex64lo(u64_t i);
# unsigned long ex64hi(u64_t i);

# Stack Layout
# 
#   esp +  8:  high 32-bit of i
#   esp +  4:  low  32-bit of i
#	esp +  0:  retAddr

	.text
	.globl	ex64lo
	.type	ex64lo, @function	
ex64lo:
	movl	4(%esp), %eax
	retl

	.globl	ex64hi
	.type	ex64hi, @function
ex64hi:
	movl	8(%esp), %eax
	retl

