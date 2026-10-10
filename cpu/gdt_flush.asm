BITS 64
global gdt_flush

gdt_flush:
    lgdt [rdi]
    
    mov ax, 0x10        ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    jmp far [cs:.flush_ptr]
    
.flush_cs:
    ret

.flush_ptr:
    dq .flush_cs
    dw 0x08