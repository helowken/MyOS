	.code16gcc
	.text
	.globl	detectLowMem
	.type	text, @function
detectLowMem:
	clc						# Clear carry flag
	xor		%eax, %eax
	int		$0x12			# Switch to the BIOS (= request low memory size)
	jc		.detectError
	jmp		.detectEnd

	.globl	detectE820Mem
	.type	text, @function
detectE820Mem:	
	pushl	%ebp
	pushl	%ebx
	movw	$0x8004, %di	
	xor		%ebx, %ebx
	xor		%ebp, %ebp
	movl	$0x534D4150, %edx
	movl	$0xe820, %eax
	movw	$1, 20(%di)
	movl	$24, %ecx
	int		$0x15
	jc		.detectError
	movl	$0x534D4150, %edx
	cmpl	%edx, %eax
	jne		.detectError
	testl	%ebx, %ebx
	je		.detectError
	jmp		.jmpin
.e820Loop:
	movw	$0xe820, %eax
	movw	$1, 20(%di)
	movl	$24, %ecx
	int		$0x15
	jc		.e820End
	movl	$0x534D4150, %edx
.jmpin:
	jcxz	.skipEntry
	cmpb	$20, %cl
	jbe		.notExtendEntry
	testb	$1, 20(%di)
	je		.skipEntry
.notExtendEntry:
	movl	8(%di), %ecx
	orl		12(%di), %ecx
	jz		.skipEntry
	inc		%ebp
	addw	$24, %di
.skipEntry:
	testl	%ebx, %ebx
	jne		.e820Loop
.e820End:
	clc
	movl	%ebp, %eax			# Return entry count.
	popl	%ebx
	popl	%ebp
	jmp		.detectEnd

.detectError:
	stc
	pushl	$.errMsg
	calll	println
	addl	$4, %esp
.detectEnd:
	retl


	.section	.rodata
.errMsg:
	.string	"Detect Memory Failed."
