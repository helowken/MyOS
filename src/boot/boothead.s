	.code16gcc
	.text
.equ	STACK_SIZE,		0x2000		
.equ	ESC,			0x1B	
.equ	DS_SELECTOR,	3*8			# Kernel data selector
.equ	ES_SELECTOR,	4*8			# Flat 4 GB
.equ	SS_SELECTOR,	5*8			# Monitor stack selector
.equ	CS_SELECTOR,	6*8			# Kernel code selector
.equ	MCS_SELECTOR,	7*8			# Monitor code selector

movw	%cs, %ax
movw	%ax, %ds		# Set ds = cs to make boot run correctly
movw	%ax, %es		# Set es = ds to clear bss

# Clear bss (we need to do it by ourselves)
xorb	%al, %al		# Fill bss with zero
movw	$edata, %di		# bss: [edata, end)
movw	$end, %cx
subw	%di, %cx		# cx = end - edata = bss size
rep	stosb


# Calculate boot run size (text + data + bss + stack)
movw	$end, %ax			# ax = text + data + bss
addw	$STACK_SIZE, %ax	# ax += stack
andw	$0xFFFE, %ax		# Round down to even (for sp)
movw	%ax, runSize


# Setup stack segment and stack pointer.
cli
movw	%ax, %sp		# sp = runSize (stack is downward)
movw	%cs, %ax		
movw	%ax, %ss		# Set ss = cs
pushw	%es				# Save es, we need it for the partition table
movw	%ax, %es		# Set es = cs (now cs, ds, es and ss all overlap)
sti
cld						


# Copy primary boot parameters to variables.
xorb	%dh, %dh
movw	%dx, device				# Boot device (probably 0x00 or 0x80)
movw	%si, bootPartEntry		# Remote partition table offset
popw	bootPartEntry+2			# and segment (saved es)


# Remember the current video mode for restoration on exit.
movb	$0x0F, %ah				# Get current video mode
int	$0x10
andb	$0x7F, %al				# Mask off bit 7 (no blanking)
movb	%al, oldVideoMode
movb	%al, currVideoMode


# Transfer segment addr to 32 bits addr and put it into caddr
xorw	%ax, %ax
movw	%cs, %dx
callw	seg2Abs
movw	%ax, caddr
movw	%dx, caddr+2


calll	boot


halt:
	jmp halt


# ================ Function Catagory ==================
# 1.  Timer 
# 2.  I/O 
# 3.  Print
# 4.  Allocate
# 5.  Device
# 6.  Read/Write
# 7.  Address
# 8.  Video Mode
# 9.  Exit
# 10. Detect memory
# 11. Bootstrap / Minix
# 12. Keyboard
# =====================================================


# =====================================================
#                1. Timer Functions 
# =====================================================
# u32_t getTick();
	.globl	getTick
	.type	getTick, @function
getTick:
	pushl	%ecx
	xorl	%eax, %eax			# ah = 0, Read System-Timer Time Counter
	xorl	%edx, %edx
	int $0x1A
	movw	%dx, %ax
	movw	%cx, %dx			# dx:ax = cx:dx = tick count, for other functions

	pushw	%dx
	pushw	%ax
	popl	%eax				# For return value

	popl	%ecx
	retl


# void pause();
	.globl	pause				
	.type	pause, @function
pause:
	hlt							# Either saves power, or tells an x86 emulator that nothing is happening right now.
	retl


# =====================================================
#                  2. I/O Functions 
# =====================================================
# int getch();
	.globl	getch
	.type	getch, @function
getch:
	xorl	%eax, %eax			# Is there a ungotten character? (previously called ungetch)
	xchgw	unchar, %ax			# Ungotten character?
	testw	%ax, %ax
	jnz	.gotch
.waitKey:
	hlt							# Play dead until interrupted (see pause())
	movb	$0x01, %ah			# Keyboard status
	int $0x16
	jz	.noKeyTyped				# Nothing typed
	xorb	%ah, %ah			# Read character from keyboard
	int $0x16
	jmp .keyPressed				# Keypress
.noKeyTyped:
	movw	line, %dx			# Serial line?
	testw	%dx, %dx			
	jz .0f
	addw	$5, %dx				# Line Status Register
	inb	%dx
	testb	$0x01, %al			# Data Ready?
	jz .0f
	movw	line, %dx
	inb	%dx						# Get character
	jmp .keyPressed
.0f:
	calll	expired				# Timer expired?
	testw	%ax, %ax			
	jz .waitKey
	movw	$ESC, %ax			# Return ESC
	retl
.keyPressed:
	cmpb	$0x0D, %al			# Is carriage typed?
	jnz	.noCarriage
	movb	$0x0A, %al			# Change carriage to newline
.noCarriage:
	cmpb	$ESC, %al			# Is ESC typed?
	jne	.noEsc
	incw	escFlag				# Set esc flag
.noEsc:
	xorb	%ah, %ah			# Make ax = al
.gotch:
	retl


# void ungetch();
	.globl	ungetch
	.type	ungetch, @function
ungetch:
	movw	4(%esp), %ax
	movw	%ax, unchar
	retl


# void putch(int ch);
	.globl	putch
	.type	putch, @function	# Same as kputc
putch:


# void kputc(int ch);
	.globl	kputc
	.type	kputc, @function
kputc:							# Called by C printf()
	movb	4(%esp), %al		# al = char 
	testb	%al, %al			# if al == 0, do nothing.
	jz	.noChar
	movb	$0xe, %ah			# BIOS ah=0xE
	cmpb	$0x9, %al			# Check tab key
	jnz	.LFKey
	movb	$0x20, %al
	int	$0x10
	int	$0x10
	int	$0x10
	jmp .putChar
.LFKey:					
	cmpb	$0xA, %al			# if al = 'LF' (new line)
	jnz	.putChar
	movb	$0xD, %al			# then output 'CR' + 'LF'
	int	$0x10
	movb	$0xA, %al
.putChar:
	int	$0x10
.noChar:
	retl


# int escape();
	.globl	escape
	.type	escape, @function	# True if ESC has been typed
escape:
	movb	$0x01, %ah			# Keybord Status
	int	$0x16
	jz	.escEnd					# If no keypress
	cmpb	$ESC, %al			
	jne	.escEnd					# If not escape typed
	xorb	%ah, %ah			# Read the escape from the keyboard buffer and discard it.
	int $0x16
	incw	escFlag
.escEnd:
	xorl	%eax, %eax
	xchgw	escFlag, %ax		# Return and reset the escape flag
	retl


# =====================================================
#                 3. Print Functions 
# =====================================================
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


# =====================================================
#               4. Allocate Functions 
# =====================================================
	.globl	brk
	.type	brk, @function
brk:
	xor	%ax, %ax
	jmp	.sbrk

	.globl	sbrk
	.type	sbrk, @function
sbrk:
	xorl	%eax, %eax
	movw	memBreak, %ax		# ax = current break
.sbrk:
	pushl	%eax				# Save it as future return value
	movl	%esp, %ebx			# Stack is now: (retValue(0), retAddr(4), increment(8))
	addw	8(%ebx), %ax		# ax = break + increment
	movw	%ax, memBreak		# Set new break
	leaw	-1024(%ebx), %dx		
	cmpw	%ax, %dx			# Compare with the new break
	jb	.heapErr
	leaw	-4096(%ebx), %dx
	cmpw	%ax, %dx			
	jae	.plenty
	pushl	$memWarn
	calll	printf				# Warn about memory running low
	addl	$4, %esp
	movb	$0, memWarn			# No more warnings
.plenty:
	popl	%eax				# Return old break (0 for brk)
	retl
.heapErr:
	pushl	$chmem
	pushl	$noMem
	calll	printf
	jmp	quit


# =====================================================
#                5. Device Functions 
# =====================================================
# int getBus();
	.globl	getBus
	.type	getBus, @function
getBus:							# Bus type: XT, AT, MCA
	xorw	%dx, %dx
	incw	%dx					# Assume AT
	movb	$0xC0, %ah			# Code for get configuration
	int	$0x15
	jc	.gotBus					# Carry clear and ah = 00h if supported
	testb	%ah, %ah			# Result is in: ES:BX -> ROM table
	jne	.gotBus
	movb	%es:5(%bx), %al		# al = Feature byte 1
	incw	%dx					# Assume MCA
	testb	$0x02, %al			# Test bit 1 - "Bus is Micro Channel"
	jnz	.gotBus
	decw	%dx					# Assume AT
	testb	$0x40, %al			# Test bit 6 - "2nd 8259 PIC installed"
	jnz .gotBus
	decw	%dx					# It is an XT
.gotBus:
	pushw	%ds
	popw	%es					# Restore es
	xorl	%eax, %eax	
	movw	%dx, %ax			# Return bus code
	movw	%ax, bus			# Keep bus code, A20 handler likes to know
	retl	


# void closeDev();
#	Close the current device. Under the BIOS this does nothing much.
	.globl	closeDev
	.type	closeDev, @function
closeDev:
	xorw	%ax, %ax
	movb	%al, devState		# State is "closed"
	retl


# int isDevBoundary(u32_t sector);
	.globl	isDevBoundary
	.type	isDevBoundary, @function
isDevBoundary:
	xorl	%eax, %eax
	movw	%sp, %bx
	xorw	%dx, %dx
	movw	6(%bx), %ax			# Divide high half of sector number (16..31 bits)
	divw	sectors				# DX:AX % sectors (sectors must be a word so that 
								# we can place remainder in dx)
	movw	4(%bx), %ax			# Divide low half of sector number (0..15 bits)
	divw	sectors				# If dx != 0, it means there is remainder for high half or low half.
	subw	$1, %dx				# If dx == 0, then CF = 1; else CF = 0
	sbbw	%ax, %ax			# ax = ax - (ax + CF) = -CF
	negl	%eax
	retl


	.type	resetDev, @function
resetDev:
	cmpb	$0, devState		# Need reset if devState < 0
	jge	.resetDevEnd		
	xorb	%ah, %ah			# Reset (ah = 0)
	movb	$0x80, %dl			# All disks (disk starts from 0x80)
	int	$0x13
	movb	$0, devState		# Set state as "closed"
.resetDevEnd:
	retl


	.type	openDev, @function
openDev:
	calll	resetDev			# Optionally reset the disks
	movb	$0, devState		# Set state as "closed"
	pushw	%es					# Save registers used by BIOS calls
	pushw	%di
	movb	device, %dl			# dl = the default device
	cmpb	$0x80, %dl			# Winchester >= 0x80
	jb	.devErr
.winchester:
	movb	$0x08, %ah			# Code for drive parameters
	int	$0x13					# dl still contains drive
	jc	.devErr				
	andb	$0x3F, %cl			# cl = max sector number (1-origin)
	incb	%dh					# dh = 1 + max head number (0-origin)
.devBoth:
	movb	%cl, sectors		# sectors per track
	movb	%cl, %al			# al = sectors per track
	mulb	%dh					# ax = heads * sectors
	movw	%ax, secsPerCyl		# Sectors per cylinder = heads * sectors
	movb	$1, devState		# Device state is "open"
	xorw	%ax, %ax			# Return code for success
.devDone:
	andl	$0xFFFF, %eax		# Clear high word
	popw	%di					# Restore di and es registers
	popw	%es
	retl
.devErr:
	movb	%ah, %al		
	xorb	%ah, %ah			# ax = BIOS error code
	jmp .devDone


# =====================================================
#              6. Read/Write Functions 
# =====================================================
# int writeSectors(char *buf, u32_t pos, int count);
	.global	writeSectors
	.type	writeSectors, @function
writeSectors:
	pushl	%ebp
	movl	%esp, %ebp
	movb	$0x03, 17(%ebp)		# Code for a disk write
	jmp	.rwSectors


# int readSectors(char *buf, u32_t pos, int count);
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
	calll	openDev				# else then initialize
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
	movw	$rwDAP, %si					# si = extended read/write DAP
	movb	16(%ebp), %cl
	movb	%cl, 2(%si)					# Fill in DAP's sectors to transfer
	movw	%bx, 4(%si)					# Buffer address offset = bx
	movw	%es, 6(%si)					# Buffer address segment = es
	movw	%ax, 8(%si)					# Starting block number low 32 bits
	movw	%dx, 10(%si)				# Starting block number high 32 bits
	movb	device, %dl					# dl = device to use
	movw	$0x4000, %ax				# Or-ed 0x02 becomes read (0x42), 0x03 becomes write (0x43)
	orw	17(%ebp), %ax					# Or-ed code for disk read (0x02) or write (0x03)
	int $0x13							# Call bios extended read/write.
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
	jnc	.rwMoreSectors					# Success reset, try again.
.rwSectorsFinish:
	movb	%ah, %al
	xorb	%ah, %ah
	andl	$0xFF, %eax					# Return value = al.
.rwSectorsEnd:
	popw %es
	popw %di
	popw %si
	leave
	retl


# void rawCopy(char *newAddr, char *oldAddr, u32_t size);
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
	adcw	$0, %cx				# add 1 to words if size is odd
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


# void relocate();
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


# =====================================================
#               7. Address Functions 
# =====================================================
# char *mon2Abs(void *pos);
	.globl	mon2Abs
	.type	mon2Abs, @function
mon2Abs:
	pushl	%ebp
	movl	%esp, %ebp
	movw	8(%ebp), %ax
	movw	%ds, %dx
	jmp	.toAbs


# char *vec2Abs(Vector *vec);
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


# =====================================================
#             8. Video Mode Functions 
# =====================================================
# https://stanislavs.org/helppc/int_10.html

# void clearScreen2();
	.globl	clearScreen2
	.type	clearScreen2, @function
clearScreen2:
	movw	$0x0600, %ax		# AH=06h, AL=0, clear entire window
	movb	$0x07, %bh			# Low 4 bits are character color and high 4 bits are background color.
	xorw	%cx, %cx
	movw	$0x184F, %dx

	int	$0x10
	movb	$0x2, %ah			# Set cursor position.
	movb	$0x0, %bh			# Display page number = 0
	movw	$0x0, %dx			# Row = dh = 0, column = dl = 0
	int	$0x10
	retl


# void setVideoMode(unsigned mode);
	.globl	setVideoMode
	.type	setVideoMode, @function
setVideoMode:
	movl	%esp, %ebx
	movl	4(%ebx), %eax		# Video mode
	cmpw	currVideoMode, %ax
	je	.videoModeOK			# Mode already as requested?
	movw	%ax, currVideoMode


# void clearScreen();
	.globl	clearScreen
	.type	clearScreen, @function
clearScreen:
	xorl	%eax, %eax
	movw	%ax, %es			# es = Vector segment
	movw	currVideoMode, %ax
	movb	%ah, %ch			# Copy of the special flags
	andb	$0x0F, %ah			# Test bits 8-11, clear special flags
	jnz	.extVesa				# VESA extended mode?
	int	$0x10					# Reset video (ah = 0)
	mfence						# Otherwise the jmp will be corruption
	jmp	.md480

# http://www.techhelpmanual.com/113-int_10h__video_services.html
.extVesa:		
	movw	%ax, %bx			# bx = extended mode
	movw	$0x4F02, %ax		# Reset video
	int	$0x10

# http://www.techhelpmanual.com/900-video_graphics_array_i_o_ports.html
.md480:							# Basic video mode is set, now build on it
	testb	$0x20, %ch			# 480 scan lines requested?
	jz	.md14pt
	movw	$0x3CC, %dx			# VGA I/O port for reading
	inb	%dx						# Read
	movb	$0xD4, %dl			# dx = 0x3D4 (CGA emulation)
	testb	$1, %al				# Mono or Color?
	jnz	0f
	movb	$0xB4, %dl			# dx = 0x3B4 (MDA emulation)

# https://wiki.osdev.org/VGA_Hardware
0:	movw	$0x110C, %ax		# Vertical Retrace end (also unlocks CR0-7)
	callw	out2
	movw	$0x060B, %ax		# Vertical total
	callw	out2
	movw	$0x073E, %ax		# (Vertical) overflow
	callw	out2
	movw	$0x10EA, %ax		# Vertical Retrace Start
	callw	out2
	movw	$0x12DF, %ax		# Vertical Display Enable End
	callw	out2
	movw	$0x15E7, %ax		# Vertical Blank Start
	callw	out2
	movw	$0x1604, %ax		# Vertical Blank End
	callw	out2
	pushw	%dx
	movb	$0xCC, %dl			# ds = 0x3CC, misc output register (read)
	inb	%dx						# Read
	movb	$0xC2, %dl			# ds = 0x3C2, (write)
	andb	$0x0D, %al			# Preserve clock select bits and color bit
	orb	$0xE2, %al				# Set correct sync polarity (0xE3)
	outb	%dx					# Write
	popw	%dx					# Restore dx = index

# http://www.techhelpmanual.com/27-dos__bios___extensions_service_index.html
.md14pt:
	testb	$0x40, %ch			# 9x14 point font requested?
	jz	.md8pt
	movw	$0x1111, %ax		# Load ROM 9x14 font
	xorb	%bl, %bl			# Load block 0
	int $0x10
	testb	$0x20, %ch			# 480 scan lines?
	jz	.md8pt
	movw	$0x12DB, %ax		# VGA vertical display end
	callw	out2
	movb	$33, %es:0x0484		# Tell BIOS the last line number
.md8pt:
	testb	$0x80, %ch			# 8x8 point font requested?
	jz	.setCursor
	movw	$0x1112, %ax		# Load ROM 8x8 font
	xorb	%bl, %bl			# Load block 0
	int $0x10
	testb	$0x20, %ch			# 480 scan lines?
	jz	.setCursor
	movw	$0x12DF, %ax		# VGA vertical display end
	callw	out2
	movb	$59, %es:0x0484		# Tell BIOS the last line number
.setCursor:
	xorw	%dx, %dx			# dl = column = 0, dh = row = 0
	xorb	%bh, %bh			# Page 0
	movb	$0x02, %ah			# Set cursor position
	int $0x10
	pushw	%ss
	popw	%es					# Restore es
.videoModeOK:
	retl


# Out to the usual [index, data] port pair that are common for VGA devices
# dx = port, ah = index, al = data.
	.type	out2, @function
out2:
	pushw	%dx
	pushw	%ax
	movb	%ah, %al
	outb	%dx			# Set index
	incw	%dx
	popw	%ax			# Restore al
	outb	%dx			# Send data
	popw	%dx
	retw


	.type	restoreVideoMode, @function
restoreVideoMode:
	pushl	oldVideoMode
	calll	setVideoMode
	addl	$4, %esp
	retl


# int getVideoMode();
	.globl	getVideoMode
	.type	getVideoMode, @function
getVideoMode:
	movw	$0x1A00, %ax		# Get Video Display Combination
	int	$0x10
	cmpb	$0x1A, %al			# If a valid function was requested in ah?
	jnz	.noDisplayCode			

	xorl	%eax, %eax
	movw	$2, %ax
	cmpb	$5, %bl				# If EGA with mnochrome display?
	jz	.gotVideo
	incw	%ax
	cmpb	$4, %bl				# If EGA with color display?
	jz	.gotVideo
	incw	%ax
	cmpb	$7, %bl				# If VGA with analog monochrome display?
	jz	.gotVideo
	incw	%ax
	cmpb	$8, %bl				# If VGA with analog color display?
	jz	.gotVideo
.noDisplayCode:
	movb	$0x12, %ah			# Get Video Subsystem Configuration
	movb	$0x10, %bl			# Return video configuration information
	int $0x10
	cmpb	$0x10, %bl			# Did it come back as 0x10? (No EGA)
	jz .noEGA

	xorl	%eax, %eax
	movw	$2, %ax
	cmpb	$1, %bh				# If mono mode in effect?
	jz	.gotVideo
	incw	%ax					# It's color mode in effect.
	jmp	.gotVideo
.noEGA:
	int $0x11					# Read Equipment-List
	andw	$0x30, %ax			# Initial video mode
	subw	$0x30, %ax			
	jz	.gotVideo				# It's monochrome (MDA).
	movw	$1, %ax				# It's color (CGA).
.gotVideo:
	retl


# =====================================================
#                9. Exit Functions 
# =====================================================
# void exit(int status);
	.globl	exit
	.type	exit, @function
exit:
	movl	%esp, %ebx
	cmpb	$0, 4(%ebx)			# Good exit status?
	jz	reboot
quit:
	pushl	$anyKey
	calll	printf
	xorb	%ah, %ah			# Read character from keyboard
	int	$0x16
reboot:
	calll	resetDev	
	calll	restoreVideoMode
	int	$0x19					# Reboot the system#


# =====================================================
#            10. Detect memory Functions 
# =====================================================
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
	pushl	$detectErrMsg
	calll	println
	addl	$4, %esp
	movl	$-1, %eax
.detectEnd:
	leave
	retl


# =====================================================
#			 11. Bootstrap / Minix Functions
# =====================================================
# void minix(u32_t kEntry, u32_t kcs, u32_t kds, 
#				char *bootParams, size_t paramSize, u32_t headerPos);
	.globl	minix
	.type	minix, @function
minix:
	cli								# Prevent interrupt before setting valid IDT.
	pushl	%ebp
	movl	%esp, %ebp
	movw	%cs, realCS				# Save real mode cs
	movw	%ds, realDS				# Save real mode ds
	movl	%cr0, %eax				
	orb	$1,	%al						# Set PE (protection enable) bit
	movl	%eax, msw				# Save to machine status
	
	movw	%ds, %dx				# Use monitor ds
	movw	$p_gdt,	%ax				# dx:ax = Global descriptor table
	callw	seg2Abs
	movw	%ax, p_gdt_desc+2		# Set base 15:00 of this GDT
	movb	%dl, p_gdt_desc+4		# Set base 23:16 of this GDT

	movw	16(%ebp), %ax			# Kernel ds (absolute address)
	movw	18(%ebp), %dx		
	movw	%ax, p_ds_desc+2		# Set base 15:00 of Kernel ds
	movb	%dl, p_ds_desc+4		# Set base 23:16 of Kenrel ds

	movw	%ss, %dx				# Use monitor ss
	xorw	%ax, %ax				# dx:ax = Monitor stack segment
	callw	seg2Abs					# Minix starts with the stack of the monitor
	movw	%ax, p_ss_desc+2		# Set base 15:00 of Kernel ss
	movb	%dl, p_ss_desc+4		# Set base 23:16 of Kenrel ss

	movw	12(%ebp), %ax			# Kernel cs (absolute address)
	movw	14(%ebp), %dx		
	movw	%ax, p_cs_desc+2		# Set base 15:00 of Kernel cs
	movb	%dl, p_cs_desc+4		# Set base 23:16 of Kenrel cs

	movw	%cs, %dx				# Monitor cs
	xorw	%ax, %ax				# dx:ax = Monitor code segment
	callw	seg2Abs
	movw	%ax, p_mcs_desc+2		# Set base 15:00 of Monitor cs
	movb	%dl, p_mcs_desc+4		# Set base 23:16 of Monitor cs

	pushw	$MCS_SELECTOR	
	pushw	$int86					# For address to INT86 support
	pushl	28(%ebp)				# Address of exec headers
	pushl	24(%ebp)				# 32 bit size of parameters on stack
	pushl	20(%ebp)				# 32 bit address of parameters (ss relative)
	pushw	$MCS_SELECTOR
	pushw	$ret386					# Monitor far return address	
	
	pushl	$CS_SELECTOR			# 32 bit for address to kernel entry point
	pushl	8(%ebp)					# cs:KernelEntry

	callw	.real2Prot				# Switch to protected mode
	movw	$DS_SELECTOR, %ax		# Kernel data
	movw	%ax, %ds
	movw	$ES_SELECTOR, %ax		# Flat 4 GB 
	movw	%ax, %es
	lret							# Make a far call to the kernel (cs:KernelEntry, 32-bits)

int86:
	#TODO
	retw

.real2Prot:
	lgdt	p_gdt_desc				# Load global descriptor table
	movl	pdbr, %eax				# Load page directory base register
	movl	%eax, %cr3			

	movl	%cr0, %eax				# Exchange real mode msw for protected mode msw 
	xchgl	%eax, msw				# msw has set PE bit
	movl	%eax, %cr0				# Set cr0 to enable protected mode

	ljmp	$MCS_SELECTOR, $.csProt		# Use a far jump to set code segment selector.
.csProt:
	movw	$SS_SELECTOR, %ax		# Set data selectors
	movw	%ax, %ss
	retw

# Switch from protected to real mode.
.prot2Real:
	lidt	p_idt_desc				# Read mode interrupt vectors
	movl	%cr3, %eax
	movl	%eax, pdbr				# Save page directory base register
	xchgl	msw, %eax				# Exchange protected mode msw for real mode msw
	movl	%eax, %cr0				# Set cr0 to disable protected mode
	pushw	realCS					# Use far return to reload cs and ip
	pushw	$.csReal
	retfw
.csReal:
	movw	realDS, %ax				# Reload data segment registers
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	retw


# Minix-386 returns here on a halt or reboot.
ret386:
	callw	.prot2Real				# Switch to real mode	

return:
	movl	%ebp, %esp				# Pop parameters
	sti								# Can take interrupts again

	calll	getVideoMode			# MDA, CGA, EGA, ...
	movb	$24, %dh				# dh = row 24
	cmpw	$2, %ax					# At least EGA?
	jb	.is25						# Otherwise 25 rows
	pushw	%ds
	xorw	%ax, %ax				# Vector & BIOS data segments
	movw	%ax, %ds
	movb	0x0484, %dh 			# dh = number of rows on display minus one
	popw	%ds
.is25:
	xorb	%dl, %dl				# dl = column 0 
	xorb	%bh, %bh				# Page 0
	movb	$0x02, %ah				# Set cursor position
	int $0x10

	movb	$-1, devState			# Minix may have upset the disks, must reset
#TODO callw	serialInit

	calll	getProcessor
	cmpw	$286, %ax
	jb	.noClock
	xorb	%al, %al
.tryClock:
	decb	%al
	jz	.noClock
	movb	$0x02, %ah				# Get real-time clock time (from CMOS clock)
	int $0x1A
	jc	.tryClock					# Carry set, not running or being updated
	movb	%ch, %al				# ch = hours in BCD
	callw	bcd						
	mulb	c60						# 60 minutes in an hour, ax = al * 60
	movw	%ax, %bx				# bx = ax = hours * 60
	movb	%cl, %al				# cl = minutes in BCD
	callw	bcd
	addw	%ax, %bx				# bx = hours * 60 + minutes
	movb	%dh, %al				# dh = seconds in BCD
	callw	bcd
	xchgw	%bx, %ax				# ax = hours * 60 + minutes, bx = seconds
	mulw	c60						# dx:ax = (hours * 60 + minutes) * 60
	addw	%ax, %bx
	adcw	$0, %dx					# dx:bx = seconds since midnight

	movw	%dx, %ax				# (0x1800B0 = ticks per day of BIOS clock)
	mulw	c19663					# dx:ax = dx * 19663  (19663 = 0x1800B0 / (2*2*2*2*5))
	xchgw	%bx, %ax				# bx = lower(ax) part of (dx * 19663)
	mulw	c19663					# dx:ax = bx * 19663
	addw	%bx, %dx				# dx:ax = dx:bx * 19663
	movw	%ax, %cx				
	movw	%dx, %ax
	xorw	%dx, %dx				# dx = 0
	divw	c1080					# ax /= 1080  (1080 = (24*60*60) / (2*2*2*2*5))
	xchgw	%cx, %ax				
	divw	c1080					# cx:ax = dx:ax / 1080
	movw	%ax, %dx				# cx:dx = ticks since midnight
	movb	$0x01, %ah				# Set system time
	int	$0x1A
.noClock:
	popl	%ebp
	retl							# Return to monitor as if nothing much happended


# Transform BCD number in al to a regular value in ax.
	.type	bcd, @function
bcd:								# ax = (al >> 4) * 10 + (al & 0x0F)	
	movb	%al, %ah	
	shrb	$4, %ah
	andb	$0x0F, %al
	aad	$10							# see "aad" for more
	retw
	

# =====================================================
#			        12. Keyboard
# =====================================================
# void scanKeyboard()
#	Read keyboard character. Needs to be done in case one is waiting.
	.globl	scanKeyboard
	.type	scanKeyboard, @function
scanKeyboard:
	inb	$0x60, %al					# Data Port
	inb	$0x61, %al					 
	movb	%al, %ah
	orb	$0x80, %al
	outb	%al, $0x61
	movb	%ah, %al
	outb	%al, $0x61
	retl




# =====================================================
#			    RO/Data/BSS Definition
# =====================================================

	.section	.rodata
detectErrMsg:
	.string	"Detect Memory Failed."
anyKey:
	.string	"\nHit any key to reboot\n"
noMem:
	.string "\nOut of%s"
memWarn:
	.ascii	"\nLow on"
chmem:
	.string " memory, use chmem to increase the heap\n"
c60:	
	.value	60
c1080:	
	.value	1080
c19663:	
	.value	19663



	.section	.data
	.globl	x_gdt			# For "Extended Memory Block Move".
	.type	x_gdt, @object
	.size	x_gdt, 48
	.align	2
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

# ------------------------------------------------------------------------

	.globl	p_gdt
	.type	p_gdt, @object	# GDT (Global Descriptor Table)
	.size	p_gdt, 64
	.align	8
p_gdt:
p_null_desc:		# The first descriptor: Null descriptor
	.zero	8
p_gdt_desc:			# Descriptor for GDT
	.value	8*8-1			# Segment limit: 15:00
							# Each entry is 8 bytes and there are 8 entries in this table.
	.zero	4				# Base address: 31:00
	.zero	2				# Alignment padding
p_idt_desc:			# Descriptor for real mode IDT (Interrupt descriptor table)
	.value	0x3FF			# Segment limit: 15:00
	.zero	4				# Base address: 31:00
	.zero	2				# Alignment padding
p_ds_desc:			# Kernel data segment descriptor (4 GB flat)
	.value	0xFFFF			# Segment limit: 15:00
	.zero	2				# Base address: 15:00
	.zero	1				# Base address: 23:16
	.byte	0x92			# P=1, DPL=0, S=1, Type=0x2=0010(RW)
	.byte	0xCF			# G=1, D/B=1, L=0, AVL=0, Segment limit(19:16)=0xF
	.zero	1				# Base address: 31:24
p_es_desc:			# Physical memory descriptor (4 GB flat)
	.value	0xFFFF			
	.zero	2				
	.zero	1				
	.byte	0x92			
	.byte	0xCF			
	.zero	1				
p_ss_desc:			# Monitor data/stack segment descriptor (64 KB flat)
	.value	0xFFFF			
	.zero	2				
	.zero	1				
	.byte	0x92
	.zero	1				# G=0, D/B=0, S=0, Segment limit(19:16)=0x0
	.zero	1				# Base address 31:24 = 0 
p_cs_desc:			# Kernel code segment descriptor (4 GB flat)
	.value	0xFFFF
	.zero	2				
	.zero	1				
	.byte	0x9A			# P=1, DPL=0, S=1, Type=0xA=1010(R/E)
	.byte	0xCF			# G=1, D/B=1, L=0, AVL=0, Segment limit(19:16)=0xF
	.zero	1				
p_mcs_desc:			# Monitor code segment descriptor (64 KB flat)
	.value	0xFFFF
	.zero	2				
	.zero	1				
	.byte	0x9A			# P=1, DPL=0, S=1, Type=0xA=1010(R/E)
	.zero	1				# G=0, D/B=0, S=0, Segment limit(19:16)=0x0
	.zero	1				
# ------------------
	.type	rwDAP, @object	# Disk Address Packet
	.size	rwDAP, 16
	.align	4
rwDAP:
	.byte	0x10			# Length of extended r/w packet
	.zero	1				# Reserved
	.zero	2				# Number of sectors to be transferred
	.zero	2				# Buffer address offset
	.zero	2				# Buffer address segment
	.zero	4				# Starting block number low 32 bits (LBA)
	.zero	4				# Starting block number high 32 bits (LBA)
# ------------------
	.type	memBreak, @object	# A fake heap pointer
	.size	memBreak, 2
	.align	2
memBreak:
	.value	end


	.section	.bss
	.lcomm	oldVideoMode, 2		# Video mode at startup
	.lcomm	currVideoMode, 2	# Current video mode
	.lcomm	devState, 2			# Device state: reset (-1), closed (0), open (1)
	.lcomm	sectors, 2			# Sectors of current device
	.lcomm	secsPerCyl, 2		# Sectors per cylinder: (sectors * heads) of current device
	.lcomm	bus, 2				# Saved retrun value of getBus
	.lcomm	unchar, 2			# Char returned by ungetch(c)
	.lcomm	escFlag, 2			# Escape typed?
	.lcomm	msw, 4				# Saved machine status (cr0)
	.lcomm	pdbr, 4				# Saved page directory base register (cr3)
	.lcomm	line, 2				# Serial line I/O port to coppy console I/O to.
	.lcomm	realDS, 2			# Saved real ds
	.lcomm	realCS, 2			# Saved real cs


