	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds
movw	%ax, %es		# Set es = ds = cs
cld						

# Clear bss
xorb	%al, %al
movw	edata, %di
movw	end, %cx
subw	%di, %cx
rep
stosb


calll	test
calll	boot


halt:
	jmp halt


#========== Print Functions ==========
	.globl	kputc
	.type	text, @function
kputc:
	movb	4(%esp), %al
	testb	%al, %al
	jz	.noChar
	movb	$0xe, %ah
	cmpb	$0xA, %al
	jnz	.putChar
	movb	$0xD, %al
	int	$0x10
	movb	$0xA, %al
.putChar:
	int	$0x10
.noChar:
	retl

	.globl	print
	.type	text, @function
print:
	movl	4(%esp), %ecx
.printLoop:
	movb	(%ecx), %al
	testb	%al, %al
	je	.printEnd
	movb	$0xe, %ah
	int	$0x10
	inc	%ecx
	jmp	.printLoop
.printEnd:
	retl

	.globl	println
	.type	text, @function
println:
	pushl	%ebp
	movl	%esp, %ebp
	movl	0x8(%ebp), %eax
	pushl	%eax
	calll	print
	movb	$0xe, %ah
	movb	$0xa, %al
	int	$0x10
	movb	$0xd, %al
	int	$0x10
	leave
	retl

#========== Addr Functions ==========
	.globl	derefSp
	.type	text, @function
derefSp:
	movl	4(%esp), %eax
	movl	%ss:(%eax), %eax
	retl

#========== Detect Memory Functions ==========
	.globl	detectLowMem
	.type	text, @function
detectLowMem:
	pushl	%ebp
	movl	%esp, %ebp
	clc						# Clear carry flag
	xor	%eax, %eax
	int	$0x12			# Switch to the BIOS (= request low memory size)
	jnc	.detectEnd
	jmp	.detectError

	.globl	detect88Mem
	.type	text, @function
detect88Mem:
	pushl	%ebp
	movl	%esp, %ebp
	clc
	xor	%eax, %eax
	movb	$0x88, %ah
	int		$0x15
	jc	.detectError
	testw	%ax, %ax
	je	.detectError
	jmp	.detectEnd

	.globl	detectE801Mem
	.type	text, @function
detectE801Mem:
	pushl	%ebp
	movl	%esp, %ebp
	xor	%ecx, %ecx
	xor %edx, %edx
	mov $0xE801, %ax
	int $0x15
	jc	.detectError
	cmpb	$0x86, %ah
	je	.detectError
	cmpb	$0x80, %ah
	je	.detectError
	testw	%cx, %cx
	jz .detectError
	movl	16(%ebp), %ebx
	movl	8(%ebp), %eax
	testl	%ebx, %ebx
	jz	.useDS
	movl	%ecx, %ss:(%eax)
	movl	12(%ebp), %eax
	movl	%edx, %ss:(%eax)
	jmp .E801End
.useDS:
	movl	%ecx, (%eax)
	movl	12(%ebp), %eax
	movl	%edx, (%eax)
.E801End:
	movl	$0, %eax
	jmp .detectEnd

	.globl	detectE820Mem
	.type	text, @function
detectE820Mem:	
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	movw	destE820, %di	
	pushl	%ebx
	xor	%ebx, %ebx
	movl	$0, -4(%ebp)
	movl	$0x534D4150, %edx
	movl	$0xe820, %eax
	movw	$1, 20(%di)
	movl	$24, %ecx
	int	$0x15
	jc	.detectError
	movl	$0x534D4150, %edx
	cmpl	%edx, %eax
	jne	.detectError
	testl	%ebx, %ebx
	je	.detectError
	jmp	.E820Check
.E820Loop:
	movl	$0xe820, %eax
	movw	$1, 20(%di)
	movl	$24, %ecx
	int	$0x15
	jc	.E820End
	movl	$0x534D4150, %edx
.E820Check:
	jcxz	.skipEntry
	cmpb	$20, %cl
	jbe	.notExtendEntry
	testb	$1, 20(%di)
	je	.skipEntry
.notExtendEntry:
	movl	8(%di), %ecx
	orl	12(%di), %ecx
	jz	.skipEntry
	incb	-4(%ebp)
	addw	$24, %di
.skipEntry:
	testl	%ebx, %ebx
	jne	.E820Loop
.E820End:
	clc
	movl	-4(%ebp), %eax			# Return entry count.
	popl	%ebx
	jmp	.detectEnd

.detectError:
	pushl	$.detectErrMsg
	calll	println
	addl	$4, %esp
	movl	$-1, %eax
.detectEnd:
	leave
	retl

	.section	.rodata
.detectErrMsg:
	.string	"Detect Memory Failed."
