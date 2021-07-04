[bits 16]

section .text

boot:
	xor	ax, ax				; ax = 0x0000, the vector segment
	mov ds, ax
	cli						; Ignore interrupts while setting stack
	mov ss, ax				; ss = ds = vector segment
	mov sp, LOAD_OFF		; Usual place for a bootstrap stack (Note: stack is downward)
	sti						; Enable interrupts again

	push	ax				; Placeholder 1 for LBA high 16 bits (bp+4)
	push	ax				; Placeholder 2 for LBA low 16 bits (bp+2)

	push	dx				; Boot device in dl will be device (bp)
	mov	bp, sp				; Use bp for navigation below.

	push	es				 
	push	si				; es:si = partition table entry if it is a hard disk

	mov	di, LOAD_OFF+sectors

winchester:
	les ax, [si+LOW_SECTOR]			; es:ax = LOW_SECTOR+2+si : LOW_SECTOR+si
	mov [bp+LOW_SEC_OFF], ax		; Store ax into placeholder 2 (bp+2)
	mov [bp+LOW_SEC_OFF+2], es		; Store es into placeholder 1 (bp+4)

	mov	ah, 0x08				
	int	0x13					; Refer to: "INT 13h and AH=08h: Read Drive Parameters" (or masterboot)			
	and	cl,	0x3F				; cl = max sector number (1-origin)
	mov [di], cl				; [di] = sectors per track
	inc	dh						; dh = 1 + max head number (0-origin)
	;jmp loadBoot

loadBoot:
; Load /boot from the boot device
	mov	al, [di]				; al = sectors per track
	mul	dh						; dh = heads, then ax = heads * sectors
	mov [bp+SEC_PER_CYL], ax	; Sectors per cylinder = ax = heads * sectors
	
	mov	ax, BOOT_SEG			; Segment to load /boot into
	mov	es, ax
	xor	bx, bx					; Load first sector at es:bx = BOOT_SEG:0x0000
	mov	si, LOAD_OFF+addresses	; Start of the boot code addresses
load:
	mov	ax, [si+1]				
	mov	dl, [si+3]
	xor	dh,	dh
	add	ax, [bp+LOW_SEC_OFF]
	add dx, [bp+LOW_SEC_OFF+2]
	cmp dx, (1024*255*63)>>16
	jae	bigDisk
	div	word [bp+SEC_PER_CYL]
	xchg	ax, dx
	mov	ch, dl
	div	byte [di]
	xor	dl, dl
	shr	dx, 1
	shr dx, 1
	or	dl, ah
	mov	cl, dl
	inc	cl
	mov	dh, al
	mov dl, [bp+DEVICE_OFF]
	mov	al, [di]
	sub	al, ah
	cmp	al, [si]
	jbe	read
	mov	al, [si]
read:
	push	ax
	mov	ah, 0x02
	int 0x13
	pop cx
	jmp readEval
bigDisk:
	; TODO
	jmp $
readEval:
	; TODO
	jmp $

mov bx, LOAD_OFF + MSG
call println_string
jmp $


%include "print/print_string.asm"
%include "print/print_hex.asm"



LOAD_OFF	equ 0x7C00	; 0x0000:LOAD_OFF is where this code is loaded
BOOT_SEG	equ	0x1000	; Secondary boot code segment
BOOT_OFF	equ	0x0030	; Offset into /boot above header
BUFFER		equ 0x0600	; First free memory
LOW_SECTOR	equ	8		; LBA (logical block addressing) of first absolute sector in the partition


; Offset addressed using bp register
DEVICE_OFF	equ 0
LOW_SEC_OFF	equ	2
SEC_PER_CYL	equ	6


MSG			db	'Starting from boot block.', 0



section .data

sectors		db	0

addresses:

