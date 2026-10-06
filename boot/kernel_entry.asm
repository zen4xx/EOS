; ===========================================================================
; EOS kernel entry point - first thing at 0x1000
;
; CPU is already in 64-bit long mode.
; First 1 GiB is identity mapped by stage2.
; ===========================================================================
BITS 64

global _start
extern kernel_main
extern __bss_start
extern __bss_end

section .text.boot progbits alloc exec

_start:
        ; -------------------------------------------------------------------
        ; Stage2 may have called us with `call rax`, so there is a return
        ; address on the stack. We do not need it. Set a clean stack.
        ; -------------------------------------------------------------------
        mov     rsp, 0x00090000
        mov     rbp, rsp
        cld

        ; -------------------------------------------------------------------
        ; Zero .bss.
        ;
        ; .bss is not stored in kernel.bin, so it must be cleared manually.
        ; On QEMU RAM is often zeroed, but on real hardware it is not.
        ; -------------------------------------------------------------------
        lea     rdi, [rel __bss_start]
        lea     rcx, [rel __bss_end]
        sub     rcx, rdi

        xor     eax, eax
        rep     stosb

        ; -------------------------------------------------------------------
        ; Call the 64-bit kernel.
        ;
        ; kernel_main must be compiled as 64-bit code.
        ; -------------------------------------------------------------------
        call    kernel_main

.hang:
        cli
        hlt
        jmp     .hang

section .note.GNU-stack noalloc noexec nowrite progbits