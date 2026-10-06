BITS 64

section .text

%macro SAVE_ALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro RESTORE_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq
%endmacro

; ==========================================================
;   sched_isr - Software interrupt handler (int 0x30)
;
;   Saves current context, calls C scheduler, switches stack,
;   restores new context and returns via iretq.
; ==========================================================

extern schedule_handler

global sched_isr
sched_isr:
    SAVE_ALL
    cld

    ; Pass current stack pointer to schedule_handler (RDI = 1st arg)
    mov rdi, rsp

    ; Align stack to 16 bytes for C function call (System V ABI requirement)
    ; Save original rsp in callee-saved rbx (will be restored from stack later)
    mov rbx, rsp
    and rsp, -16

    call schedule_handler

    ; Switch to the new stack pointer returned by schedule_handler (RAX)
    mov rsp, rax

    RESTORE_ALL


; ==========================================================
;   start_task - Start the very first task
;
;   void start_task(uint64_t rsp);
;   RDI = initial stack pointer (pointer to prepared irq_frame)
;
;   In 64-bit System V ABI, first argument is passed in RDI,
;   not on the stack like in 32-bit cdecl.
; ==========================================================

global start_task
start_task:
    mov rsp, rdi
    RESTORE_ALL


; ==========================================================
;   task_start - Entry point for new tasks
;
;   When a new task is scheduled for the first time, iretq
;   lands here. The initial irq_frame placed:
;     r12 = entry point
;     r13 = argument
;
;   System V ABI requires first argument in RDI.
; ==========================================================

extern task_exit

global task_start
task_start:
    mov rdi, r13        ; arg1 = argument
    call r12            ; entry(arg)

    ; If the task function returns, cleanly exit the task
    call task_exit

    ; Should never reach here
    ud2