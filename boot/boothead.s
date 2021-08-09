	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds		# Set ds = cs to make boot run correctly


# Clear bss (we need to do it by ourselves)
xorb	%al, %al		# Fill bss with zero
movw	$edata, %di		# bss: [edata, end)
movw	$end, %cx
subw	%di, %cx		# cx = end - edata = bss size
rep
stosb


# Calculate boot run size (text + data + bss + stack)
movw	$end, %ax			# ax = text + data + bss
addw	.stackSize, %ax		# ax += stack
andw	$0xFFFE, %ax		# Round down to even (for sp)
movw	%ax, runSize


# Setup stack segment and stack pointer
cli
movw	%ax, %sp		# sp = runSize (stack is downward)
movw	%cs, %ax		
movw	%ax, %ss		# Set ss = cs
pushw	%es				# Save es, we need it for the partition table
movw	%ax, %es		# Set es = cs (now cs, ds, es and ss all overlap)
sti
cld						


# Copy primary boot parameters to variables
xorb	%dh, %dh
movw	%dx, device				# Boot device (probably 0x00 or 0x80)
movw	%si, bootPartEntry		# Remote partition table offset
popw	bootPartEntry+2			# and segment (saved es)


# Transfer segment addr to 32 bits addr and put it into caddr
xorw	%ax, %ax
movw	%cs, %dx
callw	seg2Abs
movw	%ax, caddr
movw	%dx, caddr+2


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

#========== Dev Functions ==========
	.type	resetDev, @function
resetDev:
	cmpb	$0, devState	# Need reset if devState < 0
	jge	.resetDevEnd		
	xorb	%ah, %ah		# Reset (ah = 0)
	movb	$0x80, %dl		# All disks (disk starts from 0x80)
	int	$0x13
	movb	$0, devState	# Set state as "closed"
.resetDevEnd:
	retw

	.type	openDev, @function
openDev:
	callw	resetDev		# Optionally reset the disks
	movb	$0, devState	# Set state as "closed"
	pushw	%es				# Save registers used by BIOS calls
	pushw	%di
	movb	device, %dl		# dl = the default device
	cmpb	$0x80, %dl		# Winchester >= 0x80
	jb	.devErr
.winchester:
	movb	$0x08, %ah		# Code for drive parameters
	int	$0x13				# dl still contains drive
	jc	.devErr				
	andb	$0x3F, %cl		# cl = max sector number (1-origin)
	incb	%dh				# dh = 1 + max head number (0-origin)
.devBoth:
	movb	%cl, sectors		# sectors per track
	movb	%cl, %al			# al = sectors per track
	mulb	%dh					# ax = heads * sectors
	movw	%ax, secsPerCyl		# Sectors per cylinder = heads * sectors
	movb	$1, devState		# Device state is "open"
	xorw	%ax, %ax			# Return code for success
.devDone:
	andl	$0xFFFF, %eax	# Clear high word
	popw	%di				# Restore di and es registers
	popw	%es
	retw
.devErr:
	movb	%ah, %al		
	xorb	%ah, %ah		# ax = BIOS error code
	jmp .devDone
	
#========== Read/Write Functions ==========
	.global	writeSectors
	.type	writeSectors, @function
writeSectors:
	pushl	%ebp
	movl	%esp, %ebp
	movb	$0x03, 17(%ebp)		# Code for a disk write
	jmp	.rwSectors

	.globl	readSectors
	.type	readSectors, @function
readSectors:
	pushl	%ebp
	movl	%esp, %ebp
	movb	$0x02, 17(%ebp)		# Code for a disk read
								# ebp[16..19] = sectors to transfer, we just use one byte for sectors (ebp[16]),
								# so we can use ebp[17] to store the op code (read or write)
.rwSectors:
	pushw	%si					# Save registers
	pushw	%di
	pushw	%es
	cmpb	$0, devState		# if device is opened?
	jg	.devOpened
	callw	openDev				# else then initialize
	testw	%ax, %ax			# if return code != 0, then go error
	jnz	.rwSectorsEnd			
.devOpened:
	movw	8(%ebp), %ax
	movw	10(%ebp), %dx
	callw	abs2Seg				# dx:ax = dx-ax = buffer address
	movw	%ax, %bx
	movw	%dx, %es			# es:bx = dx:ax = buffer address
	movw	$1, %di				# Only 1 reset on hard disk error
	cmpl	$0, 16(%ebp)		# if count == 0, then go to end.
	jz	.rwSectorsDone
.rwMoreSectors:
	movw	12(%ebp), %ax
	movw	14(%ebp), %dx				# dx:ax = abs sector (LBA)
	cmpw	(1024*255*63)>>16, %dx		# Near 8G limit?
	jae	.rwSectorsBigDisk
	divw	secsPerCyl					# ax = cylinder, dx = sectors within cylinder
	xchgw	%dx, %ax					# dx = cylinder, ax = sectors within cylinder
	movb	%dl, %ch					# ch = low 8 bits of cylinder
	divb	sectors						# al = head, ah = sector (0-origin)
	xorb	%dl, %dl					# About to shift bits 8-9 of cylinder into dl
	shrw	$1, %dx
	shrw	$1, %dx						# dl[6..7] = high cylinder
	orb	%ah, %dl						# dl[0..5] = sector (0-origin)
	movb	%dl, %cl					# cl[0..5] = sector (0-origin), cl[6..7] = high cynlinder
	incb	%cl							# cl[0..5] = sector (1-origin)
	movb	%al, %dh					# dh = head
	movb	device, %dl					# dl = device to use
	movb	sectors, %al
	subb	%ah, %al					# Sectors per track - sector number (0-origin) = sectors left on this track
	cmpb	16(%ebp), %al				# if sectors left on this track <= sectors to transfer,
	jbe .rwSectorsDoit					# then transfer sectors left on this track first
	movb	16(%ebp), %al				# else then just transfer sectors by parameter
.rwSectorsDoit:
	movb	17(%ebp), %ah				# Code for disk read (0x02) or write (0x03)
	pushw	%ax							# Save al = sectors to transfer
	int	$0x13
	popw	%cx							# Restore al in cl
	jmp .rwSectorsEval
.rwSectorsBigDisk:

.rwSectorsEval:
	jc	.rwSectorsIoErr					# if there is I/O error, then go error, else means ah = 0
	movb	%cl, %al					# Restore al = cl = sectors to transfer
	addb	%al, %bh					# bx += 2 * al * 256 (add bytes transferred)
	addb	%al, %bh					# es:bx = where next sector is located
	addw	%ax, 12(%ebp)				# Update address by sectors transferred
	adcw	$0,	14(%ebp)				# if carry, increase high word
	subb	%al, 16(%ebp)				# Decrement sector count by sectors transferred
	jnz	.rwMoreSectors					# if not all sectors have been transferred
.rwSectorsDone:
	xorb	%ah, %ah
	jmp .rwSectorsFinish
.rwSectorsIoErr:
	cmpb	$0x80, %ah					# Disk timed out?
	je	.rwSectorsFinish
	cmpb	$0x03, %ah					# Disk write protected?
	je	.rwSectorsFinish
	decw	%di							# Can we try again?
	jl	.rwSectorsFinish				# No, report error
	xorb	%ah, %ah
	int $0x13
	jnc	.rwMoreSectors					# Success reset, try again
.rwSectorsFinish:
	movb	%ah, %al
	xorb	%ah, %ah
.rwSectorsEnd:
	popw %si
	popw %di
	popw %es
	leave
	retl


	.globl	rawCopy
	.type	rawCopy, @function
rawCopy:
	pushl	%ebp
	movl	%esp, %ebp
	pushw	%si
	pushw	%di
.copy:
	cmpw	$0, 18(%ebp)		# High 16 bits of remaining size (init value is runSize)
	jnz	.bigCopy				# if remaining size >= 64K
	movw	16(%ebp), %cx		# Low 16 bits of remaining size (init value is runSize)
	test	%cx, %cx
	jz	.copyDone				# If low 16 bits == 0 => remaining size = 0
	cmpw	$0xFFF0, %cx
	jb	.smallCopy
.bigCopy:
	movw	$0xFFF0, %cx		# Don't copy more than about 64K at once
.smallCopy:
	pushw	%cx
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
	popw	%cx					# Restore copyCount
	addw	%cx, 8(%ebp)		# dstAddr += copyCount
	adcw	$0, 10(%ebp)		# if cx + 8(%ebp) > 0xFFFF, then 10(%ebp) += 1
	addw	%cx, 12(%ebp)		# srcAddr += copyCount
	adcw	$0,	14(%ebp)		# if cx + 12(%ebp) > 0xFFFF, then 14(%ebp) += 1
	subw	%cx, 16(%ebp)		# count -= copyCount
	sbbw	$0, 18(%ebp)		# if 16(%ebp) - cx < 0, then 18(%ebp) -= 1
	jmp	.copy
.copyDone:
	popw	%di					# Restore di and si
	popw	%si
	leave
	retl

	.globl	relocate
	.type	relocate, @function
relocate:
	popl	%ebx				# Save return address
	movw	caddr, %ax
	movw	caddr+2, %dx
	callw	abs2Seg				# Transfer 32 bits dx-ax to segment dx:ax
	movw	%dx, %cx			# cx = new code segment
	movw	%cs, %ax			# ax = old code segment
	movswl	%cx, %ecx			# if cx == 0x9000 && ax == 0x1000,
	movswl	%ax, %eax			# Without signed extension: ax - cx = 0x8000,
								# With signed extension: eax - ecx = 0xFFFF8000
	subl	%ecx, %eax			# eax = eax - ecx = old - new = -(moving offset)
	movw	%ds, %dx			# dx = ds
	movswl	%dx, %edx
	subl	%eax, %edx			# edx - eax => edx - -(moving offset) => edx += moving offset
	movw	%dx, %ds			# We just need low 16 bits of edx
	movw	%dx, %es			
	movw	%dx, %ss			# ds = es = ss = dx
	pushw	%cx					# New text segment
	pushw	%bx					# Return offset of this function
	retfw						# Return far with cs = cx and ip = bx

#========== Address Functions ==========
	.globl	mon2Abs
	.type	mon2Abs, @function
mon2Abs:
	pushl	%ebp
	movl	%esp, %ebp
	movw	8(%ebp), %ax
	movw	%ds, %dx
	jmp	.toAbs

	.globl	vec2Abs
	.type	vec2Abs, @function
vec2Abs:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %ebx
	movw	(%ebx), %ax
	movw	2(%ebp), %dx
	jmp	.toAbs

.toAbs:
	callw	seg2Abs
	pushw	%dx
	pushw	%ax
	popl	%eax
	leave
	retl

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
.stackSize:
	.long	0x2800


	.section	.data
	.local	devState		# Device state: reset (-1), closed (0), open (1)
	.comm	devState,1,1
	.local	sectors			# Sectors of current device
	.comm	sectors,1,1
	.local	secsPerCyl		# Sectors per cylinder: (sectors * heads) of current device
	.comm	secsPerCyl,2,2	
	
	.globl	x_gdt			# For "Extended Memory Block Move".
	.align	2
	.type	x_gdt, @object
	.size	x_gdt, 48
x_gdt:						
x_null_desc:				# Dummy Descriptor. Initialized by user to 0.
	.zero	8				
x_gdt_desc:					# Descriptor of this GDT. Initialized by user to 0. Modified by BIOS.
	.zero	8
x_src_desc:					# Descriptor of Source block. Initialized by user.
	.value	0xFFFF			# Word containing segment limit
	.value	0				# Low word of 24-bit address
	.zero	1				# High byte of 24-bit address
	.byte	0x93			# Access rights byte
	.zero	2				# Reserved (must be 0)
x_dst_desc:					# Descriptor of Destination block. Initialized by user.
	.value	0xFFFF			# Similar as x_src_desc
	.value	0
	.zero	1
	.byte	0x93
	.zero	2
x_bios_desc:				# Descriptor for Protected Mode Code Segment. Initialized by user to 0. Modified by BIOS.
	.zero	8		
x_ss_desc:					# Descriptor for Protected Mode Stack Segment. Initialized by user to 0. Modified by BIOS.
	.zero	8



