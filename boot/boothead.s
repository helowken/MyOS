	.code16gcc
	.text
cli
movw	%cs, %ax
movw	%ax, %ss
movw	%ax, %ds
movw	%ax, %es		# Set es = ds = ss = cs
sti
cld						

# Clear bss
xorb	%al, %al
movw	edata, %di
movw	end, %cx
subw	%di, %cx
rep
stosb

# Transfer segment addr to 32 bits addr and put it into caddr
xorw	%ax, %ax
movw	%cs, %dx
callw	seg2Abs
leaw	caddr, %bx
movw	%ax, (%bx)
movw	%dx, 2(%bx)


calll	test
calll	boot


halt:
	jmp halt


#========== Print Functions ==========
	.globl	kputc
	.type	kputc, @function
kputc:							# Called by C printf()
	movb	4(%esp), %al		# al = char 
	testb	%al, %al			# if al == 0, do nothing.
	jz	.noChar
	movb	$0xe, %ah			# Call BIOS ah=0xE
	cmpb	$0xA, %al			# if al = 'LF' (new line)
	jnz	.putChar
	movb	$0xD, %al			# then output 'CR' + 'LF'
	int	$0x10
	movb	$0xA, %al
.putChar:
	int	$0x10
.noChar:
	retl

	.globl	print
	.type	print, @function
print:
	movl	4(%esp), %ecx		# ecx = address of string
.printLoop:
	movb	(%ecx), %al			# al = char of string
	testb	%al, %al			# if al == 0 => ends of string
	je	.printEnd
	movb	$0xe, %ah			# Call BIOS ah=0xE
	int	$0x10
	inc	%ecx					# Go to the next char
	jmp	.printLoop
.printEnd:
	retl

	.globl	println
	.type	println, @function
println:
	pushl	%ebp
	movl	%esp, %ebp
	movl	0x8(%ebp), %eax
	pushl	%eax
	calll	print				# Call print
	movb	$0xe, %ah			
	movb	$0xd, %al			# Output 'CR'
	int	$0x10
	movb	$0xa, %al			# Output 'LF'
	int	$0x10
	leave
	retl

#========== Copy Functions ==========
	.globl	rawCopy
	.type	rawCopy, @function
rawCopy:
	pushl	%ebp
	movl	%esp, %ebp
	pushw	%si
	pushw	%di
.copy:
	cmpw	$0, 18(%ebp)		# High 16 bits of runSize
	jnz	.bigCopy
	movw	16(%ebp), %cx		# Low 16 bits of runSize (because high 16 bits == 0)
	test	%cx, %cx
	jz	.copyDone			# If low 16 bits == 0 => runSize = 0
	cmpw	$0xFFF0, %cx
	jb	.smallCopy
.bigCopy:
	movw	$0xFFF0, %cx		# Don't copy more than about 64K at once
.smallCopy:
	push	%cx
	movw	8(%ebp), %ax		# Low 16 bits of dest addr
	movw	10(%ebp), %dx		# High 16 bits of dest addr
	cmpw	$0x0010, %dx		# Copy to extended memory?
								# Base addr of extended memory between 1M and 16M is 0x100000
								# See: BIOS Function: INT 0x15, AX = 0xE801
	jae	.extendCopy
	cmpw	$0x0010, 14(%ebp)	# Copy from extended memory? (High 16 bits of src addr)
	jae .extendCopy
	callw	abs2Seg				# (32 bits addr) dx-ax = (segment addr) dx:ax
	movw	%ax, %di
	movw	%dx, %es			# es:di = dx:ax = dest addr
	movw	12(%ebp), %ax		# Low 16 bits of src addr
	movw	14(%ebp), %dx		# High 16 bits of dest addr
	callw	abs2Seg				# (32 bits addr) dx-ax = (segment addr) dx:ax
	movw	%ax, %si
	movw	%dx, %ds			# ds:si = dx:ax = src addr
	shrw	$1,	%cx				# cx / 2 = words to move
	rep
	movsw						# Do the word copy
	adcw	%cx, %cx			# if (cx % 2 != 0) then cx = 1
	rep
	movsb						# Do one more byte copy
	movw	%ss, %ax			# Restore ds and es from the remaining ss
	movw	%ax, %ds
	movw	%ax, %es			# es = ds = ss
	jmp	.copyAdjust
.extendCopy:
	movw	%ax, x_dst_desc+2	# Low word of 24-bit destination address
	movb	%dl, x_dst_desc+4	# High byte of 24-bit destination address
	movw	12(%ebp), %ax		
	movw	14(%ebp), %dx		
	movw	%ax, x_src_desc+2	# Low word of 24-bit source address
	movb	%dl, x_src_desc+4	# High byte of 24-bit source address
	movw	$x_gdt, %si			# es:si = global descriptor table
	shrw	$1, %cx				# Words to move
	movb	$0x87, %ah	
	int	$0x15
.copyAdjust:
	popw	%cx
	addw	%cx, 8(%ebp)		# dstAddr += copyCount
	adcw	$0, 10(%ebp)		# if cx + 8(%ebp) > 0xFFFF, then 10(%ebp) += 1
	addw	%cx, 12(%ebp)		# srcAddr += copyCount
	adcw	$0,	14(%ebp)		# if cx + 12(%ebp) > 0xFFFF, then 14(%ebp) += 1
	subw	%cx, 16(%ebp)		# count -= copyCount
	sbbw	$0, 18(%ebp)		# if 16(%ebp) - cx < 0, then 18(%ebp) -= 1
	jmp	.copy
.copyDone:
	popw	%di
	popw	%si
	leave
	retl

#========== Addr Functions ==========
	.type	abs2Seg, @function
abs2Seg:								# Transfer the 32 bit address dx-ax to dx:ax
	pushw	%cx
	movb	%dl, %ch
	movw	%ax, %dx
	andw	$0x000F, %ax
	movb	$4, %cl
	shrw	%cl, %dx
	shlb	%cl, %ch
	orb	%ch, %dh
	popw	%cx
	retw

	.type seg2Abs, @function
seg2Abs:
	pushw	%cx
	movb	%dh, %ch
	movb	$4, %cl
	shlw	%cl, %dx
	shrb	%cl, %ch
	addw	%dx, %ax
	adcb	$0, %ch
	movb	%ch, %dl
	xorb	%dh, %dh
	popw	%cx
	retw

#========== Detect Memory Functions ==========
	.globl	detectLowMem
	.type	detectLowMem, @function
detectLowMem:
	pushl	%ebp
	movl	%esp, %ebp
	clc						# Clear carry flag
	xor	%eax, %eax
	int	$0x12			# Switch to the BIOS (= request low memory size)
	jnc	.detectEnd
	jmp	.detectError

	.globl	detect88Mem
	.type	detect88Mem, @function
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
	.type	detectE801Mem, @function
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
	.type	detectE820Mem, @function
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


	.section	.data
	.globl	x_gdt
# For "Extended Memory Block Move".
	.align	2
	.type	x_gdt, @object
	.size	x_gdt, 48
x_gdt:						
x_null_desc:				# Dummy Descriptor. 
							# Initialized by user to 0.
	.zero	8				
x_gdt_desc:					# Descriptor of this GDT. 
							# Initialized by user to 0. 
							# Modified by BIOS.
	.zero	8
x_src_desc:					# Descriptor of Source block.
							# Initialized by user.
	.value	0xFFFF			# Word containing segment limit
	.value	0				# Low word of 24-bit address
	.zero	1				# High byte of 24-bit address
	.byte	0x93			# Access rights byte
	.zero	2				# Reserved (must be 0)
x_dst_desc:					# Descriptor of Destination block.
							# Initialized by user.
	.value	0xFFFF			# Similar as x_src_desc
	.value	0
	.zero	1
	.byte	0x93
	.zero	2
x_bios_desc:				# Descriptor for Protected Mode Code Segment.
							# Initialized by user to 0. 
							# Modified by BIOS.
	.zero	8		
x_ss_desc:					# Descriptor for Protected Mode Stack Segment.
							# Initialized by user to 0. 
							# Modified by BIOS.
	.zero	8



