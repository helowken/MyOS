	.text
.equ SEND,		1		
.equ RECEIVE,	2
.equ SENDREC,	3
.equ NOTIFY,	4
.equ ECHO,		8
.equ NB_SEND,		0x10 + SEND		# Flags 0x10 to prevent blocking
.equ NB_RECEIVE,	0x10 + RECEIVE	# Flags 0x10 to prevent blocking
.equ SYSVEC,	33	# Trap to kernel

.equ SRC_DST,	8	# Source / destination process
.equ ECHO_MESS,	8	# Doesn't have SRC_DST
.equ MESSAGE,	12	# Message pointer


	.globl	_send
	.type	_send, @function
_send:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = dest
	movl	MESSAGE(%ebp), %ebx		# ebx = message pointer
	movl	$SEND, %ecx				# _send(dst, msg)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl

	.globl	_nbSend
	.type	_nbSend, @function
_nbSend:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = dest
	movl	MESSAGE(%ebp), %ebx		# ebx = message pointer
	movl	$NB_SEND, %ecx			# _nbSend(dst, msg)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl

	.globl	_receive
	.type	_receive, @function
_receive:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = src
	movl	MESSAGE(%ebp), %ebx		# ebx = message pointer
	movl	$RECEIVE, %ecx			# _receive(src, msg)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl

	.globl	_nbReceive
	.type	_nbReceive, @function
_nbReceive:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = src
	movl	MESSAGE(%ebp), %ebx		# ebx = message pointer
	movl	$NB_RECEIVE, %ecx		# _nbReceive(src, msg)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl

	.globl	_sendRec
	.type	_sendRec, @function
_sendRec:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = src-dest
	movl	MESSAGE(%ebp), %ebx		# ebx = message pointer
	movl	$SENDREC, %ecx			# _sendRec(srcDst, msg)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl
	
	.globl	_notify
	.type	_notify, @function
_notify:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	SRC_DST(%ebp), %eax		# eax = dest
	movl	$NOTIFY, %ecx			# _notify(dst)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl
	
	.globl	_echo
	.type	_echo, @function
_echo:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	ECHO_MESS(%ebp), %ebx	# ebx = message pointer
	movl	$ECHO, %ecx				# _echo(msg)
	int	$SYSVEC						# trap to the kernel
	popl	%ebx
	popl	%ebp
	retl








