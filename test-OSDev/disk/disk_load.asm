; load DH sectors to ES:BX from drive DL
disk_load:
	push dx

	mov ah, 0x2
	mov al, dh
	mov ch, 0x0
	mov cl, 0x2
	mov dh, 0x0

	int 0x13

	jc disk_error

	pop dx
	cmp dh, al
	jne disk_read_failed
	ret

disk_error:
	mov bx, DISK_ERROR_MSG
	call print_string
	jmp $

disk_read_failed:
	mov bx, DISK_READ_FAILED_MSG
	call print_string
	jmp $

DISK_ERROR_MSG:
	db 'Disk read error!', 0

DISK_READ_FAILED_MSG:
	db 'Disk read failed!', 0
