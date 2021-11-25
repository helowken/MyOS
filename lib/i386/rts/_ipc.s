	.text
.equ SEND,		1		
.equ RECEIVE,	2
.equ SENDREC,	3
.equ NOTIFY,	4
.equ ECHO,		8

.equ SYSVEC,	33	# Trap to kernel

.equ SRC_DST,	8	# Source / destination process
.equ MESSAGE,	12	# Message pointer


	.globl	_receive
	.type	_receive, @function
_receive:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = src-dest
	movl	MESSAGE(%ebp), %ebx		# ebx = message pointer
	movl	$RECEIVE, %ecx			# _receive(src, ptr)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	leave
	retl
