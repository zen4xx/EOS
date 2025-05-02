gdt_start: 
    dd 0x0 ; 4 byte
    dd 0x0 ; 4 byte

gdt_code: 
    dw 0xFFFF    ; segment length, bits 0-15
    dw 0x0000    ; segment base, bits 0-15
    db 0x0000    ; segment base, bits 16-23
    db 10011010b ; flags (8 bits)
    db 11001111b ; flags (4 bits) + segment length, bits 16-19
    db 0x0000    ; segment base, bits 24-31

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x0000
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size (16 bit), always one less of its true size
    dd gdt_start ; address (32 bit)

; define some const
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
