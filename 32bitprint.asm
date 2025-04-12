[bits 32]

VIDEO_MEMORY equ 0xb8000
COLOR equ 0x1f

print_string_pm:
	pusha
	mov edx, VIDEO_MEMORY

print_string_pm_loop:
	mov al, [ebx] ; [ebx] address for characters
	mov ah, COLOR
	
	cmp al, 0
	je print_string_pm_done

	mov [edx], ax ; char + vm attribute
	add ebx, 1 ; next char
	add edx, 2 ; next vm postion

	jmp print_string_pm_loop

print_string_pm_done:
	popa
	ret
