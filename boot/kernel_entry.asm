; ===========================================================================
; EOS kernel entry point - first thing at 0x1000
; ===========================================================================
BITS 32

global _start
extern kernel_main
extern __bss_start
extern __bss_end

section .text.boot

_start:
        mov     esp, 0x00090000
        mov     ebp, esp

        ; -------------------------------------------------------------------
        ; Zero the .bss.
        ;
        ; .bss is *not* stored in kernel.bin - the loader only copies bytes
        ; that exist in the file. QEMU hands the guest zeroed RAM, so every
        ; uninitialised global happened to be 0 and the kernel looked fine.
        ; A real BIOS leaves whatever it was using down there, so
        ; interrupt_handlers[], idt[], free_list_head and friends come up
        ; full of garbage. interrupt_handlers[] alone is fatal: the first
        ; IRQ then calls a random address.
        ; -------------------------------------------------------------------
        cld
        mov     edi, __bss_start
        mov     ecx, __bss_end
        sub     ecx, edi
        xor     eax, eax
        rep     stosb

        call    kernel_main

.hang:
        cli
        hlt
        jmp     .hang

section .note.GNU-stack noalloc noexec nowrite progbits
