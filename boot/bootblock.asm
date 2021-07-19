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
	mov	al, [di]					; al = sectors per track
	mul	dh						    ; dh = heads, then ax = heads * sectors
	mov [bp+SEC_PER_CYL], ax	    ; Sectors per cylinder = ax = heads * sectors
	
	mov	ax, BOOT_SEG			    ; Segment to load /boot into
	mov	es, ax
	mov bx, BOOT_OFF				; Load first sector at es:bx = BOOT_SEG:0x0000
	mov	si, LOAD_OFF+addresses	    ; Start of the boot code addresses
									; [si] = sectors of the boot code
									; [si+1, si+2] = low 16 bits of the boot code address
									; [si+3] = high 8 bits of the boot code address
load:
	mov	ax, [si+1]				    ; Get next sector number of the boot code address: low 16 bits
	mov	dl, [si+3]				    ; Bits 16-23 for our up to 8 GB partition
	xor	dh,	dh					    ; dx:ax = sector within partition. Since CHS just uses 24 bits, dh should be cleared.
	add	ax, [bp+LOW_SEC_OFF]	    ; Add LBA (of the first sector in this partition) to the boot code address
	add dx, [bp+LOW_SEC_OFF+2]	    ; dx:ax = sector within drive
	cmp dx, (1024*255*63)>>16	    ; Near 8G limit?
	jae	bigDisk
	div	word [bp+SEC_PER_CYL]	    ; ax = cylinder, dx = sector within cylinder
	xchg	ax, dx				    ; ax = sector within cylinder, dx = cylinder
	mov	ch, dl					    ; ch = low 8 bits of cylinder
	div	byte [di]				    ; al = head, ah = sector (0-origin)
	xor	dl, dl					    ; About to shift bits 8-9 of cylinder into dl
	shr	dx, 1					 
	shr dx, 1					    ; dl[6..7] = high cylinder
	or	dl, ah					    ; dl[0..5] = sector (0-origin)
	mov	cl, dl					    ; cl[0..5] = sector (0-origin), cl[6..7] = high cylinder
	inc	cl						    ; cl[0..5] = sector (1-origin)
	mov	dh, al					    ; dh = al = header
	mov dl, [bp+DEVICE_OFF]		    ; dl = device to read
	mov	al, [di]				    ; al = sectors per track
	sub	al, ah						; Sectors per track - Sector number (0-origin) = Sectors left on this track
	cmp	al, [si]					; Compare
	jbe	read						; If sectors left on this track <= sectors of the boot code, go to read.
	mov	al, [si]					; Otherwise read only sectors of the boot code.
read:
	push	ax						; Save al = sectors to read
	mov	ah, 0x02					; Code for disk read (All registers have been set values)
	int 0x13						; Call the BIOS for a read
	pop cx							; Restore al in cl
	jmp readEval					
bigDisk:
	mov	cl, [si]					; Number of sectors to read
	push	si						; Save si
	mov si, LOAD_OFF+DAP			; si = DAP 
	mov [si+2], cl					; Number of sectors to be read
	mov [si+4], bx					; Buffer address
	mov [si+8], ax					; Sector start to read
	mov [si+10], dx
	mov	dl, [bp+DEVICE_OFF]			; dl = device to read
	mov ah, 0x42					; Extended read
	int 0x13
	pop	si							; Restore si
readEval:
	jc	readError			; Jump on disk read error
	mov	al, cl				; Restore al = sectors read
							; es:bx is the buffer address pointer, after read succeed, 
							; there are al * 512 bytes in this buffer, so bx is updated to point to a new position.
							; bx + al * 512 = bx + al * 2 * 256
							;			    = bx + ((al * 2) >> 8)
							;				= (bh + al * 2) : bl
							;				= (bh + al) + (bh + al) : bl
	add	bh, al				
	add	bh, al				; es:bx = where next sector will be stored
	add [si+1], ax			; Update the boot code address by sectors read
	adc	[si+3], ah			; Since read succeed, ah = 0
							; If a carry occured when computing [si+1] + ax, then [si+3] needs to add 1.
	sub	[si], al			; [si] = sectors of the boot code left to read
	jnz load				; Not all sectors of the boot code have been read
	add si, 4				; Next (count, address) pair
	cmp ah, [si]			; Now ah = 0, if [si] == 0, then done.
	jnz load				; Read next chunk of the boot code.
done:
	pop si							; Restore es:si = partition table entry
	pop es							; Now dl is still loaded with device
	jmp word BOOT_SEG:BOOT_OFF		; Jump to the boot code.
readError:
	mov bx, LOAD_OFF+ERR_READ
	call print_string

	mov bx, LOAD_OFF
	mov dh, ah
	call print_hex

	call println
	jmp $



%include "print/print_string.asm"
%include "print/print_hex.asm"


LOAD_OFF	equ 0x7C00	; 0x0000:LOAD_OFF is where this code is loaded
BOOT_SEG	equ	0x1000	; Secondary boot code segment
BOOT_OFF	equ	0x0000	; Offset into /boot 
LOW_SECTOR	equ	8		; LBA (logical block addressing) of first absolute sector in the partition


; Offset addressed using bp register
DEVICE_OFF	equ 0
LOW_SEC_OFF	equ	2
SEC_PER_CYL	equ	6


ERR_READ	db	'Read failed: ', 0


section .data
DAP:					; DAP: Disk Address Packet
						; Refer to INT 13h AH=42h: Extended Read Sectors From Drive
		db 0x10			; size of DAP
		db 0			; Reserved
		dw 0			; number of sectors to be read
		dw 0			; Buffer address offset
		dw BOOT_SEG		; Buffer address segment

						; Since LBA in partition entry has only 4 bytes,
						; just low part will be filled
		dd 0			; low 32 bits of LBA
		dd 0			; high 32 bits of LBA

sectors	db 0

align 2
addresses:

