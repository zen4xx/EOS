[org 0x7c00] ; bootloader offset
KERNEL_OFFSET equ 0x1000
	
	mov [BOOT_DRIVE], dl
	mov bp, 0x9000 ; set the stack
	mov sp, bp

	mov bx, MSG_REAL_MODE
	call print 
	call print_nl
	
	call load_kernel
	call switch_to_pm
	jmp $ ; infinite loop

%include "bs_print.asm"
%include "bs_print_hex.asm"
%include "32bit_gdt.asm"
%include "32bit_print.asm"
%include "32bit_switch.asm"
%include "bs_disk.asm"

[bits 16]
load_kernel:
	mov bx, MSG_LOAD_KERNEL
	call print
	call print_nl
	
	mov bx, KERNEL_OFFSET
	mov dh, 2
	mov dl, [BOOT_DRIVE]
	call disk_load
	ret

[bits 32]
BEGIN_PM: ; after the switch we will get here
	mov ebx, MSG_PROT_MODE
	call print_string_pm 
	call KERNEL_OFFSET
	jmp $

BOOT_DRIVE db 0
MSG_REAL_MODE db "Started in 16-bit real mode", 0
MSG_PROT_MODE db "Loaded 32-bit protected mode", 0
MSG_LOAD_KERNEL db "Loading kernel in memory", 0

; bootsector
times 510-($-$$) db 0
dw 0xaa55
