[org 0x7c00]
mov ah, 0x0e

mov bx, string
call print

jmp $ ; infinite loop

%include "bs_print.asm"

string:
	db 'hello world', 0

times 510-($-$$) db 0
; magic number
dw 0xaa55
