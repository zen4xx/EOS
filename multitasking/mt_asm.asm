; mt.asm

BITS 32

section .text

%macro SAVE_ALL 0
    pushad
    push ds
    push es
    push fs
    push gs
%endmacro

%macro RESTORE_ALL 0
    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd
%endmacro

; ==========================================================
;   int 0x30
; ==========================================================

extern schedule_handler

global sched_isr
sched_isr:
    SAVE_ALL
    cld

    mov eax, esp

    push eax

    call schedule_handler
    add esp, 4

    mov esp, eax

    RESTORE_ALL


global start_task
start_task:
    mov eax, [esp + 4]
    mov esp, eax

    RESTORE_ALL

; ==========================================================
;   ebx = entry
;   ecx = arg
;
;   void task_entry(void *arg);
; ==========================================================

extern task_exit

global task_start
task_start:
    ; entry(arg)
    push ecx
    call ebx
    add esp, 4

    call task_exit

    ud2