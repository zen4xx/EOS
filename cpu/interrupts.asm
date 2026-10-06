BITS 64

; External C handlers
extern isr_handler
extern irq_handler
extern syscall_handler

; ---------------------------------------------------------------------------
; Common stub for ISR (CPU exceptions)
; ---------------------------------------------------------------------------
isr_common_stub:
    ; Save all general-purpose registers
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    ; Save data segment (for compatibility, though ignored in long mode)
    mov     rax, ds
    push    rax

    ; Pass pointer to register frame as first argument (RDI)
    mov     rdi, rsp

    ; Call C handler
    call    isr_handler

    ; Restore data segment
    pop     rax
    mov     ds, ax
    mov     es, ax

    ; Restore all general-purpose registers
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    ; Remove int_no and error_code from stack (16 bytes total)
    add     rsp, 16

    ; Return from interrupt (64-bit version)
    iretq

; ---------------------------------------------------------------------------
; Common stub for IRQ (hardware interrupts)
; ---------------------------------------------------------------------------
irq_common_stub:
    ; Save all general-purpose registers
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    ; Save data segment
    mov     rax, ds
    push    rax

    ; Pass pointer to register frame as first argument (RDI)
    mov     rdi, rsp

    ; Call C handler
    call    irq_handler

    ; Restore data segment
    pop     rax
    mov     ds, ax
    mov     es, ax

    ; Restore all general-purpose registers
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    ; Remove int_no and error_code from stack
    add     rsp, 16

    ; Return from interrupt
    iretq

; ---------------------------------------------------------------------------
; ISR declarations
; ---------------------------------------------------------------------------
global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr31
global isr80

; IRQ declarations
global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

; ---------------------------------------------------------------------------
; ISR stubs (CPU exceptions 0-31)
; ---------------------------------------------------------------------------

; 0: Divide By Zero Exception
isr0:
    cli
    push    0               ; fake error code
    push    0               ; interrupt number
    jmp     isr_common_stub

; 1: Debug Exception
isr1:
    cli
    push    0
    push    1
    jmp     isr_common_stub

; 2: Non Maskable Interrupt Exception
isr2:
    cli
    push    0
    push    2
    jmp     isr_common_stub

; 3: Int 3 Exception
isr3:
    cli
    push    0
    push    3
    jmp     isr_common_stub

; 4: INTO Exception
isr4:
    cli
    push    0
    push    4
    jmp     isr_common_stub

; 5: Out of Bounds Exception
isr5:
    cli
    push    0
    push    5
    jmp     isr_common_stub

; 6: Invalid Opcode Exception
isr6:
    cli
    push    0
    push    6
    jmp     isr_common_stub

; 7: Coprocessor Not Available Exception
isr7:
    cli
    push    0
    push    7
    jmp     isr_common_stub

; 8: Double Fault Exception (With Error Code!)
isr8:
    cli
    push    8               ; error code already pushed by CPU
    jmp     isr_common_stub

; 9: Coprocessor Segment Overrun Exception
isr9:
    cli
    push    0
    push    9
    jmp     isr_common_stub

; 10: Bad TSS Exception (With Error Code!)
isr10:
    cli
    push    10
    jmp     isr_common_stub

; 11: Segment Not Present Exception (With Error Code!)
isr11:
    cli
    push    11
    jmp     isr_common_stub

; 12: Stack Fault Exception (With Error Code!)
isr12:
    cli
    push    12
    jmp     isr_common_stub

; 13: General Protection Fault Exception (With Error Code!)
isr13:
    cli
    push    13
    jmp     isr_common_stub

; 14: Page Fault Exception (With Error Code!)
isr14:
    cli
    push    14
    jmp     isr_common_stub

; 15: Reserved Exception
isr15:
    cli
    push    0
    push    15
    jmp     isr_common_stub

; 16: Floating Point Exception
isr16:
    cli
    push    0
    push    16
    jmp     isr_common_stub

; 17: Alignment Check Exception
isr17:
    cli
    push    0
    push    17
    jmp     isr_common_stub

; 18: Machine Check Exception
isr18:
    cli
    push    0
    push    18
    jmp     isr_common_stub

; 19: Reserved
isr19:
    cli
    push    0
    push    19
    jmp     isr_common_stub

; 20: Reserved
isr20:
    cli
    push    0
    push    20
    jmp     isr_common_stub

; 21: Reserved
isr21:
    cli
    push    0
    push    21
    jmp     isr_common_stub

; 22: Reserved
isr22:
    cli
    push    0
    push    22
    jmp     isr_common_stub

; 23: Reserved
isr23:
    cli
    push    0
    push    23
    jmp     isr_common_stub

; 24: Reserved
isr24:
    cli
    push    0
    push    24
    jmp     isr_common_stub

; 25: Reserved
isr25:
    cli
    push    0
    push    25
    jmp     isr_common_stub

; 26: Reserved
isr26:
    cli
    push    0
    push    26
    jmp     isr_common_stub

; 27: Reserved
isr27:
    cli
    push    0
    push    27
    jmp     isr_common_stub

; 28: Reserved
isr28:
    cli
    push    0
    push    28
    jmp     isr_common_stub

; 29: Reserved
isr29:
    cli
    push    0
    push    29
    jmp     isr_common_stub

; 31: Reserved
isr31:
    cli
    push    0
    push    31
    jmp     isr_common_stub

; ---------------------------------------------------------------------------
; System call handler (ISR 0x80)
;
; In 64-bit System V ABI, arguments are passed in registers:
;   RDI = arg1
;   RSI = arg2
;   RDX = arg3
;   RCX = arg4
;   R8  = arg5
;   R9  = arg6
;
; The syscall number is typically in RAX.
; ---------------------------------------------------------------------------
isr80:
    cli

    ; Save all general-purpose registers
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    ; Pass pointer to saved registers as first argument
    mov     rdi, rsp
    call    syscall_handler

    ; syscall_handler returns result in rax
    ; Update the saved rax on the stack so it gets restored correctly
    mov     [rsp + 14*8], rax

    ; Restore all registers
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    sti
    iretq

; ---------------------------------------------------------------------------
; IRQ stubs (hardware interrupts 0-15, mapped to ISR 32-47)
; ---------------------------------------------------------------------------

irq0:
    cli
    push    0               ; fake error code
    push    32              ; interrupt number
    jmp     irq_common_stub

irq1:
    cli
    push    0
    push    33
    jmp     irq_common_stub

irq2:
    cli
    push    0
    push    34
    jmp     irq_common_stub

irq3:
    cli
    push    0
    push    35
    jmp     irq_common_stub

irq4:
    cli
    push    0
    push    36
    jmp     irq_common_stub

irq5:
    cli
    push    0
    push    37
    jmp     irq_common_stub

irq6:
    cli
    push    0
    push    38
    jmp     irq_common_stub

irq7:
    cli
    push    0
    push    39
    jmp     irq_common_stub

irq8:
    cli
    push    0
    push    40
    jmp     irq_common_stub

irq9:
    cli
    push    0
    push    41
    jmp     irq_common_stub

irq10:
    cli
    push    0
    push    42
    jmp     irq_common_stub

irq11:
    cli
    push    0
    push    43
    jmp     irq_common_stub

irq12:
    cli
    push    0
    push    44
    jmp     irq_common_stub

irq13:
    cli
    push    0
    push    45
    jmp     irq_common_stub

irq14:
    cli
    push    0
    push    46
    jmp     irq_common_stub

irq15:
    cli
    push    0
    push    47
    jmp     irq_common_stub