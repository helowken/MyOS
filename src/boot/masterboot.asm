; This code may be placed in the first sector (the boot sector) of a floppy,
; hard disk or hard disk primary partition.  There it will perform the
; following actions at boot time:
;
; - If the booted device is a hard disk and one of the partitions is active
;   then the active partition is booted.
;
; - Otherwise the next floppy or hard disk device is booted, trying them one
;   by one.
;
; To make things a little clearer, the boot path might be:
;	/dev/fd0	- Floppy disk containing data, tries fd1 then d0
;	[/dev/fd1]	- Drive empty
;	/dev/c0d0	- Master boot block, selects active partition 2
;	/dev/c0d0p2	- Submaster, selects active subpartition 0
;	/dev/c0d0p2s0	- Minix bootblock, reads Boot Monitor /boot
;	Minix		- Started by /boot from a kernel image in /minix


[bits 16]

section .text

master:
	xor ax, ax
	mov ds, ax
	mov es, ax
	cli					; Before changing the stack, disable interrupt
	mov ss, ax			; ds = es = ss = Vector segment
	mov sp, LOAD_OFF	; Note: stack is downward, it ranges from [0x0500, LOAD_OFF) where 0x0500 is the first free memory
						; Refer to "Typical lower memory layout after boot" in "os-dev".
	sti					; After the stack is changed, enable interrupt again

; Copy this code to safety, then jump to it.
	mov	si, sp						; si = start of this code
	push	si						; Also where we'll return to eventually
	mov di, BUFFER					; Buffer area 
	mov cx, 512						; One sector
	cld					
	rep movsb						; Copy this code from LOAD_OFF to BUFFER.
									; When finished, area [LOAD_OFF, LOAD_OFF + 512) and [BUFFER, BUFFER + 512) have the same code.

	jmp word 0x0:BUFFER+migrate     ; Take a far jump to safety.
									; When finished, the CPU will be directed from 'jmp' in [LOAD_OFF, LOAD_OFF + 512)
									; to the 'migrate' in [BUFFER, BUFFER + 512).
									; Then [LOAD_OFF, LOAD_OFF + 512) can be used as an area for loading boot block code.
migrate:

; Find the active partition
findActive:
	test	dl, dl					; BIOS stores the drive code in dl, 0x00, 0x01 ... means floppy, 0x80, 0x81 ... means hard drive
	jns errorBootDisk				; If it is not a hard dirve, jump to error
	mov si,	BUFFER+PART_TABLE		; Read the partition table in MBR
find:
	cmp	byte [si+PTYPE], 0			; Partition type, nonzero when in use
	jz	nextPart
	test byte [si+PSTATUS], 0x80	; Active partition flag in bit 7
	jz	nextPart
loadPart:
	call	load					; Load partition bootstrap (boot block)
	jc	readError					; If CF is set, jump to error
	ret								; Ret will pop the return address from stack, 
									; that return address is LOAD_OFF which was pushed on stack by si,
									; after load, LOAD_OFF contains the boot block code, to which the control now is transferred
nextPart:
	add si, PENTRY_SIZE
	cmp	si, BUFFER+PART_TABLE+4*PENTRY_SIZE		; Partition table contains 4 entries, each has 16 bytes.
	jb	find
	
	mov bx, BUFFER+ERR_NO_ACTIVE_PART			; No active partition
	call	println_string
	jmp reboot
load:
	mov di, 3			; Setup 3 retries for floppy spinup
retry:
	push	dx
	push	es
	push	di			; Next call will destroy es and di, refer to: "INT 13h and AH=08h: Read Drive Parameters"
	mov	ah,	0x08
	int 0x13			; Drive Parameters Results:
						; CF: Set On Error, Clear If No Error
						; AH: Return Code
						; DL: number of hard disk drives
						; DH: logical last index of heads = number_of - 1 (because index starts with 0)
						; CX: [7:6][15:8] logical last index of cylinders = number_of - 1 (because index starts with 0)
						;	  [5:0] logical last index of sectors per track = number_of (because index starts with 1)
						; BL: drive type (only AT/PS2 floppies)
						; ES:DI pointer to drive parameter table (only for floppies)
	pop	di
	pop es
	jc	readError		; If CF is set, jump to error
	
	and	cl, 0x3F		; Refer to CX[5:0] described above, compute cl = sectors per track
	inc	dh				; Refer to DH described above, compute dh = heads
	mov	al, cl
	mul	dh				; ax = al * dh = sectors * heads
	mov	bx, ax			; bx = sectors per cylinder = sectors * heads

	mov ax,	[si+LOW_SECTOR]			; Get LBA of first absolute sector in the partition.
	mov dx, [si+LOW_SECTOR+2]		; Now si points to the partition entry, 
									; and [si+LOW_SECTOR] points to the LBA which has 4 bytes,
									; so make DX:AX to represent the LBA: AX=bytes[0,1], DX=bytes[2,3].
									; Refer to "Partition table entries in MBR"

	cmp	dx, (1024*255*63)>>16	; Near 8G limit? 
								; BIOS disk I/O uses 24-bit CHS (cylinder-head-sector) 
								; which scheme is 10:8:6, or 1024 cylinders, 256 heads, and 63 sectors.
								; Since sector size is 512 bytes (= 9 bits), then 24 + 9 = 33 bits = 8 GB. 
								; (Note: cylinder and head index start with 0 while sector index starts with 1)
								;
								; ATA uses 28-bit CHS which scheme is 16:4:8.
								; A translation scheme called "large or bit shift translation" remaps ATA cylinders and heads to 24-bit CHS,
								; increasing the practical limit to 1024*256*63 sectors, or 8.4G.
								;
								; Refer to LBA: "Logical block addressing"
								; 
	jae bigDisk					; Function 02h of INT 13h may only read sectors of the first 16,450,560(=1024*255*63) sectors of your hard drive,
								; to read sectors beyond the 8GB limit you should use function 42h of INT 13h Extensions.

	div bx				; Refer to LBA: "Logical block addressing"
						; CHS tuples can be mapped to LBA address with the following formula:
						; LBA = (C * HPC + H) * SPT + (S - 1)
						; where
						; # C, H and S are the cylinder number, the head number, and the sector number
						; # LBA is the logical block address
						; # HPC is the maximum number of heads per cylinder (refer to DH of Drive Parameters Results aboved)
						; # SPT is the maximum number of sectors per track (refer to CX[5:0] of Drive Parameters Results aboved)
						;
						; LBA addresses can be mapped to CHS tuples with the following formula:
						; # C = LBA / (HPC * SPT)
						; # H = (LBA / SPT) % HPC
						; # S = (LBA % SPT) + 1
						;
						; Here, LBA = DX:AX, bx = sectors * heads,
						; "div bx" => DX:AX / bx 
						;		   => DX:AX / (sectors * heads) 
						;          => LBA / (HPC * SPT) 
						;		   => C (cylinder number)
						;		   => AX is the quotient and DX is the remainder
						;		   => AX = C
						;			  DX = H * SPT + (S - 1)   (sector within cylinder)
						
	xchg	ax, dx		; Make ax = sector within cylinder, dx = cylinder
						
						; When using INT 13h and AH=02h to read sectors from drive,
						; CX contains both the cylinder number (10 bits, possible values are 0 to 1024) 
						; and the sector number (6 bits, possible values are 1 to 63).
						; Cylinder and Sector bits are numbered below:
						;
						;		CX =		  ---CH--- ---CL---
						;		cylinder	: 76543210 98
						;		sector		:			 543210
						;
						;
						; Now:	DX =		  ---DH--- ---DL---
						;							98 76543210
	mov	ch, dl			; Make ch = low 8 bits of cylinder 
						;		CX =		  ---CH--- ---CL---
						;					  76543210

	div	cl				; "div cl" => ax / cl 
						;		   => sector within cylinder / sectors per track
						;		   => (H * SPT + S - 1) / SPT
						;		   => H (head number)
						;		   => AL is the quotient and AH is the remainder
						;		   => AL = H
						;			  AH = S - 1
						
	xor	dl,	dl			; Clear dl.
	shr	dx, 1			;
	shr dx, 1			;		DX =		  ---DH--- ---DL---
						;							   98
	or	dl, ah			;							   98(S - 1)
						;		CX =		  ---CH--- ---CL---
	mov	cl, dl			;					  
	inc cl				;					  76543210 98( S  )
						
						; INT 13h AH=02h: Read Sectors From Drive
						;		Parameters:
						; AH		02h
						; AL		Sectors to Read Count
						; CH		Cylinder
						; CL		Setor
						; DH		Head
						; DL		Drive
						; ES:BX		Buffer Address Pointer
						;
						;		Results:
						; CF		Set On Error, Clear If No Error
						; AH		Return Code
						; AL		Actual Sectors Read Count
						;
	pop dx				; Restore drive code in dl
	mov	dh,	al			; Make dh = head
	mov bx, LOAD_OFF	; es:bx = where sector is loaded
	mov ax, 0x0201		; ah = 02h (function code), al = 01 (read just one sector)
	int 0x13
	jmp readEval		; Evaluate read result
bigDisk:
							; INT 13h AH=42h: Extended Read Sectors From Drive				
							; AH		42h
							; DL		drive index (e.g. 1st HDD = 80h)
							; DS:SI		segment:offset pointer to the DAP, see below
							;
							;					DAP: Disk Address Packet
							;	offset range  |   size	 |		description
							;	00h			  | 1 byte	 | size of DAP (set this to 10h)
							;	01h			  |	1 byte	 | unused, should be zero
							;	02h..03h	  | 2 bytes	 | number of sectors to be read
							;	04h..07h	  | 4 bytes  | segment:offset pointer to the memory buffer to which sectors will be transfered
							;							   (note that x86 is little-endian: if declaring the segment and offset separately, 
							;							   the offset must be declared before the segment)
							;	08h..0Fh	  | 8 bytes  | absolute number of the start of the sectors to be read 
							;							   (1st sector of drive has number 0) using logical block addressing 
							;							   (note that the lower half comes before the upper half)
							;
	mov bx, dx				; bx:ax = dx:ax = sector to read (refer to "Get LBA in partition entry" aboved)
	pop dx					; Restore drive code in dl
	push	si				; Save si
	mov	si, BUFFER+DAP		; Make si points to DAP
							;
							; Put partition entry's LBA into DAP's LBA
	mov [si+8], ax			; Because of little-endian, ax goes first, then bx
	mov	[si+10], bx			; 
	mov	ah,	0x42			
	int 0x13			
	pop si					; Restore si to point to partition entry
readEval:
	jnc	readOK			; Read succeeded
	cmp	ah, 0x80		; Disk timed out? (Floppy drive empty)
	je	readBad			
	dec di				; Decrease retry count
	jl	readBad			; If retry count expired, jump to error
	xor	ah, ah
	int 0x13			; Reset Disk System
	jnc retry			; Try again
readBad:
	stc					; set CF to represent error
	ret
readOK:
	cmp word [LOAD_OFF+MAGIC], 0xAA55		; Check Boot Signature, (0x55 0xAA in disk) with little-endian => 0xAA55
	jne	noSig								; If check signature failed, go error
	ret
noSig:
	mov bx, BUFFER+ERR_NO_SIGNATURE
	call println_string
	ret
errorBootDisk:
	mov bx, BUFFER+ERR_BOOT_DISK
	call println_string
	jmp reboot
readError:
	mov bx, BUFFER+ERR_READ
	call print_string

	mov bx, BUFFER
	mov dh, ah
	call print_hex

	call println
	jmp reboot
reboot:
	mov bx, BUFFER+MSG_REBOOT
	call println_string
	xor	ah, ah					; Wait for keypress
	int 0x16
	call println
	int 0x19					; Reboot


%include "print/print_string.asm"
%include "print/print_hex.asm"


LOAD_OFF	equ 0x7C00	; 0x0000:LOAD_OFF is where this code is loaded
BUFFER		equ 0x0600	; First free memory to store this code
MAGIC		equ	510		; Location of the AA55 magic number
PART_TABLE	equ	446		; Location of partition table within this code
PENTRY_SIZE	equ	16		; Size of one partition table entry
PTYPE		equ	4		; Partition type offset
PSTATUS		equ 0		; Status for active or bootable
LOW_SECTOR	equ	8		; LBA (logical block addressing) of first absolute sector in the partition


; Messages to display
MSG_REBOOT	db	'Hit any key to reboot.', 0
ERR_READ	db	'Read failed: ', 0
ERR_BOOT_DISK:
		db 'Not HDD.', 0
ERR_NO_ACTIVE_PART:
		db 'No active part.', 0
ERR_NO_SIGNATURE:
		db 'No bootable.', 0


section .data
DAP:					; DAP: Disk Address Packet
						; Refer to INT 13h AH=42h: Extended Read Sectors From Drive
		db 0x10			; size of DAP
		db 0			; Reserved
		dw 1			; number of sectors to be read
		dw LOAD_OFF		; Buffer address offset
		dw 0			; Buffer address segment

						; Since LBA in partition entry has only 4 bytes,
						; just low part will be filled
		dd 0			; low 32 bits of LBA
		dd 0			; high 32 bits of LBA

