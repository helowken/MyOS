# unsigned long div64u(u64_t i, unsigned j);
#
# Stack Layout
#   esp + 12:  j
#   esp +  8:  high 32-bit of i
#   esp +  4:  low  32-bit of i
#	esp +  0:  retAddr
#
# edx:eax / memory => eax(quotient), edx(remainder)
# Since the return type is unsigned long, the quotient in high 32-bit is thrown away.

	.text
	.globl	div64u
	.type	div64u, @function
div64u:
	xorl	%edx, %edx
	movl	8(%esp), %eax		# i = (i-high << 32) + i-low
	divl	12(%esp)			# i-high = quotient * j + remainder
	movl	4(%esp), %eax		# eax = low 32-bit, edx = remainder in high 32-bit
	divl	12(%esp)			# i / j = (quotient << 32) + ((remainder << 32) + i-low) / j
	retl
