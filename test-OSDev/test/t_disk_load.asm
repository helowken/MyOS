[bits 16]
[org 0x7C00]

mov [BOOT_DRIVE], dl

mov bp, 0x8000
mov sp, bp

mov bx, 0x9000
mov dh, 2
mov dl, [BOOT_DRIVE]
call disk_load

mov dx, [0x9000]
call print_hex
call print_nl

mov dx, [0x9000 + 512]
call print_hex

jmp $

%include "../print/print_string.asm"
%include "../print/print_hex.asm"
%include "../disk/disk_load.asm"

BOOT_DRIVE:
	db 0

times 510 - ($-$$) db 0
dw 0xaa55

dw 0xdada
times 510 db 0

dw 0xface
times 510 db 0
