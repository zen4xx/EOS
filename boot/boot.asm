[org 0x7c00]
	mov bp, 0x8000 ; set the stack
	mov sp, bp
	
	mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x9000
	mov dh, 2

	call disk_load ; bios sets dl for boot disk num

	mov dx, [0x9000] ; retrieve the first loaded word 0xdada
	call print_hex
	call print_nl

	mov dx, [0x9000 + 512] ; first word from second loader sector 0xface
	call print_hex


	jmp $ ; infinite loop

%include "bs_print.asm"
%include "bs_print_hex.asm"
%include "bs_disk.asm"

times 510-($-$$) db 0
; magic number
dw 0xaa55

times 256 dw 0xabcd
times 256 dw 0xedac
