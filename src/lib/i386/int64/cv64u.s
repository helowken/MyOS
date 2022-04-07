# unsigned cv64u(u64_t i);
# unsigned long cv64ul(u64_t i);
#
# Stack layout:
#   esp +  8:  high 32-bit of i
#   esp +  4:  low 32-bit of i
#	esp +  0:  retAddr

	.text
	.globl	cv64u
	.type	cv64u, @function
cv64u:

	.globl	cv64ul
	.type	cv64ul, @function
cv64ul:
	movl	4(%esp), %eax
	cmpl	$0,	8(%esp)			# Return ULONG_MAX if really big
	jz		0f
	movl	$-1, %eax
0:
	retl
