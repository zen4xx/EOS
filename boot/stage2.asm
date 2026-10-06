; ===========================================================================
; EOS - stage 2 loader
; loaded by stage 1 at 0000:7E00, padded to 4 sectors
;
; Modified: enter 64-bit long mode before calling the kernel.
;
; NOTE:
;   * the kernel at 0x1000 must be 64-bit code
;   * first 1 GiB is identity mapped with 2 MiB pages
;   * page tables are placed after stage2/kernel image
; ===========================================================================
BITS 16
ORG 0x7E00

%define KERNEL_OFF      0x1000          ; where the kernel is linked
%define KERNEL_SEG      0x0100          ; 0x0100:0x0000 == physical 0x1000
%define KERNEL_LBA      5               ; 1 stage1 + 4 stage2
%define STACK_TOP       0x00090000      ; protected/long mode stack

%ifndef KERNEL_SECTORS                  ; the Makefile should pass real value
%define KERNEL_SECTORS 64
%endif

%assign KERNEL_END KERNEL_OFF + KERNEL_SECTORS * 512
%assign STAGE2_END 0x7E00 + 4 * 512

%if KERNEL_END > STAGE2_END
%assign FREE_RAW KERNEL_END
%else
%assign FREE_RAW STAGE2_END
%endif

%assign PT_BASE ((FREE_RAW + 0xFFF) & 0xFFFFF000)
%assign PML4    PT_BASE
%assign PDPT    PT_BASE + 0x1000
%assign PD      PT_BASE + 0x2000

%if PT_BASE + 0x3000 > STACK_TOP
%error "EOS stage2: kernel image too large for fixed page-table/stack layout"
%endif

stage2:
            cli
            xor     ax, ax
            mov     ds, ax
            mov     es, ax
            mov     ss, ax
            mov     sp, 0x7C00
            sti
            cld
            mov     [boot_drive], dl

            mov     ax, 0x0003              ; 80x25 colour text, clears screen
            int     0x10

            mov     si, msg_hello
            call    puts

            call    detect_ext
            call    get_geometry
            call    load_kernel
            call    enable_a20

            mov     si, msg_pm
            call    puts

            cli
            lgdt    [gdt_descriptor]

            ; Enter 32-bit protected mode first.
            ; Long mode requires paging and PAE, so we prepare in 32-bit mode.
            mov     eax, cr0
            or      eax, 1
            mov     cr0, eax
            jmp     CODE32_SEG:pm32_entry

; ===========================================================================
; detect_ext - is INT 13h AH=42h (LBA read) available?
; ===========================================================================
detect_ext:
            mov     byte [use_lba], 0
            mov     ah, 0x41
            mov     bx, 0x55AA
            mov     dl, [boot_drive]
            int     0x13
            jc      .no
            cmp     bx, 0xAA55
            jne     .no
            test    cl, 1                   ; bit 0 = extended read/write
            jz      .no
            mov     byte [use_lba], 1
.no:
            ret

; ===========================================================================
; get_geometry - INT 13h AH=08h
; ===========================================================================
get_geometry:
            push    es
            mov     ah, 0x08
            mov     dl, [boot_drive]
            xor     di, di
            mov     es, di                  ; ES:DI = 0:0 works around buggy BIOSes
            int     0x13
            jc      .fallback

            and     cl, 0x3F                ; CL[5:0] = sectors per track
            jz      .fallback
            xor     ax, ax
            mov     al, cl
            mov     [spt], ax

            xor     ax, ax
            mov     al, dh                  ; DH = max head index
            inc     ax
            jz      .fallback
            mov     [heads], ax
            pop     es
            ret

.fallback:
            mov     word [spt], 18          ; floppy-shaped default
            mov     word [heads], 2
            test    byte [boot_drive], 0x80
            jz      .out
            mov     word [spt], 63          ; hard-disk-shaped default
            mov     word [heads], 16
.out:
            pop     es
            ret

; ===========================================================================
; load_kernel - read KERNEL_SECTORS sectors from LBA KERNEL_LBA to 0x1000
; ===========================================================================
load_kernel:
            mov     word [cur_lba], KERNEL_LBA
            mov     word [left], KERNEL_SECTORS
            mov     word [dst_seg], KERNEL_SEG

.next:
            cmp     word [left], 0
            je      .done

            cmp     byte [use_lba], 0
            je      .chs_prep
            mov     ax, 127                 ; DAP limit
            jmp     .have_max

.chs_prep:
            call    calc_chs
            mov     ax, [spt]               ; sectors remaining on this track
            mov     bx, [chs_cx]
            and     bx, 0x003F
            sub     ax, bx
            inc     ax

.have_max:
            cmp     ax, [left]
            jbe     .cap_boundary
            mov     ax, [left]

.cap_boundary:
            mov     bx, [dst_seg]           ; never cross a 64K DMA boundary
            and     bx, 0x0FFF
            mov     si, 0x1000
            sub     si, bx
            shr     si, 5                   ; paragraphs -> sectors
            cmp     ax, si
            jbe     .cap_ok
            mov     ax, si

.cap_ok:
            mov     [count], al

            cmp     byte [use_lba], 0
            je      .do_chs
            call    read_lba
            jnc     .read_ok
            mov     byte [use_lba], 0       ; BIOS lied about extensions
            jmp     .next

.do_chs:
            call    read_chs
            jc      .fail

.read_ok:
            mov     al, '.'                 ; progress, so a slow drive does
            call    putc                    ; not look like a hang

            xor     ax, ax
            mov     al, [count]
            add     [cur_lba], ax
            sub     [left], ax
            mov     bx, ax
            shl     bx, 5                   ; sectors * 512 / 16 paragraphs
            add     [dst_seg], bx
            jmp     .next

.done:
            mov     si, msg_crlf
            call    puts
            ret

.fail:
            mov     si, msg_disk
            call    puts
            jmp     halt

; ---------------------------------------------------------------------------
; calc_chs - [cur_lba] -> [chs_cx], [chs_dh]
calc_chs:
            mov     ax, [cur_lba]
            xor     dx, dx
            div     word [spt]              ; AX = lba/spt   DX = lba%spt
            mov     cl, dl
            inc     cl                      ; sector is 1-based
            xor     dx, dx
            div     word [heads]            ; AX = cylinder  DX = head
            mov     ch, al                  ; cylinder low 8 bits
            shl     ah, 6                   ; cylinder bits 8..9 -> CL[7:6]
            or      cl, ah
            mov     [chs_cx], cx
            mov     [chs_dh], dl
            ret

; ---------------------------------------------------------------------------
; read_chs / read_lba - CF set if all retries failed
; ---------------------------------------------------------------------------
read_chs:
            mov     di, 5
.retry:
            mov     ax, [dst_seg]
            mov     es, ax
            xor     bx, bx
            mov     ah, 0x02
            mov     al, [count]
            mov     cx, [chs_cx]
            mov     dh, [chs_dh]
            mov     dl, [boot_drive]
            int     0x13
            jnc     .ok
            call    disk_reset
            dec     di
            jnz     .retry
            stc
            ret
.ok:
            clc
            ret

read_lba:
            mov     di, 5
.retry:
            mov     byte [dap + 0], 0x10    ; packet size
            mov     byte [dap + 1], 0
            xor     ax, ax
            mov     al, [count]
            mov     [dap + 2], ax           ; sector count
            mov     word [dap + 4], 0       ; buffer offset
            mov     ax, [dst_seg]
            mov     [dap + 6], ax           ; buffer segment
            mov     ax, [cur_lba]
            mov     [dap + 8], ax           ; LBA low
            mov     word [dap + 10], 0
            mov     word [dap + 12], 0
            mov     word [dap + 14], 0

            mov     ah, 0x42
            mov     dl, [boot_drive]
            mov     si, dap
            int     0x13
            jnc     .ok
            call    disk_reset
            dec     di
            jnz     .retry
            stc
            ret
.ok:
            clc
            ret

disk_reset:
            pusha
            xor     ah, ah
            mov     dl, [boot_drive]
            int     0x13
            popa
            ret

; ===========================================================================
; A20
; ===========================================================================
enable_a20:
            call    a20_test
            jnz     .ok

            mov     ax, 0x2401              ; 1) ask the BIOS nicely
            int     0x15
            call    a20_test
            jnz     .ok

            in      al, 0x92                ; 2) fast A20
            test    al, 2
            jnz     .skip92
            or      al, 2
            and     al, 0xFE                ; bit0 = fast reset, never set it
            out     0x92, al
.skip92:
            call    a20_test
            jnz     .ok

            call    a20_kbc                 ; 3) the 8042, if there is one
            call    a20_test
            jnz     .ok

            mov     si, msg_a20
            call    puts
.ok:
            ret

; returns ZF=0 when A20 is enabled
a20_test:
            push    ds
            push    es
            push    si
            push    di
            xor     ax, ax
            mov     es, ax
            mov     di, 0x0500
            mov     ax, 0xFFFF
            mov     ds, ax
            mov     si, 0x0510              ; FFFF:0510 aliases 0000:0500

            mov     al, [es:di]
            push    ax                      ; save original 0000:0500
            mov     al, [ds:si]
            push    ax                      ; save original FFFF:0510

            mov     byte [es:di], 0x00
            mov     byte [ds:si], 0xFF
            mov     dl, [es:di]             ; 0xFF -> write wrapped -> A20 off

            pop     ax
            mov     [ds:si], al
            pop     ax
            mov     [es:di], al

            cmp     dl, 0xFF                ; flags survive pops below
            pop     di
            pop     si
            pop     es
            pop     ds
            ret                             ; ZF=1 when wrapped (A20 off)

a20_kbc:
            cli
            call    kbc_wait_in
            jc      .out
            mov     al, 0xAD
            out     0x64, al                ; disable keyboard

            call    kbc_wait_in
            jc      .out
            mov     al, 0xD0
            out     0x64, al                ; read output port
            call    kbc_wait_out
            jc      .out
            in      al, 0x60
            mov     bl, al

            call    kbc_wait_in
            jc      .out
            mov     al, 0xD1
            out     0x64, al                ; write output port
            call    kbc_wait_in
            jc      .out
            mov     al, bl
            or      al, 2
            out     0x60, al

            call    kbc_wait_in
            jc      .out
            mov     al, 0xAE
            out     0x64, al                ; re-enable keyboard
            call    kbc_wait_in
.out:
            sti
            ret

; CF=1 on timeout
kbc_wait_in:
            mov     cx, 0xFFFF
.l:
            in      al, 0x64
            test    al, 2
            jz      .ready
            loop    .l
            stc
            ret
.ready:
            clc
            ret

kbc_wait_out:
            mov     cx, 0xFFFF
.l:
            in      al, 0x64
            test    al, 1
            jnz     .ready
            loop    .l
            stc
            ret
.ready:
            clc
            ret

; ===========================================================================
halt:
            cli
            hlt
            jmp     halt

putc:
            pusha
            mov     ah, 0x0E
            xor     bx, bx
            int     0x10
            popa
            ret

puts:
            pusha
            mov     ah, 0x0E
            xor     bx, bx
.l:
            lodsb
            test    al, al
            jz      .d
            int     0x10
            jmp     .l
.d:
            popa
            ret

; ===========================================================================
; GDT
;
; We need:
;   - 32-bit code segment to prepare page tables and enable long mode
;   - data segment
;   - 64-bit code segment for the final long mode jump
; ===========================================================================
gdt_start:
            dq 0x0000000000000000

gdt_code32:
            dw 0xFFFF
            dw 0x0000
            db 0x00
            db 10011010b
            db 11001111b
            db 0x00

gdt_data:
            dw 0xFFFF
            dw 0x0000
            db 0x00
            db 10010010b
            db 11001111b
            db 0x00

gdt_code64:
            dw 0xFFFF
            dw 0x0000
            db 0x00
            db 10011010b
            db 00100000b                    ; L=1, D=0, G=0
            db 0x00

gdt_end:

gdt_descriptor:
            dw gdt_end - gdt_start - 1
            dd gdt_start

CODE32_SEG  equ gdt_code32 - gdt_start
DATA_SEG    equ gdt_data   - gdt_start
CODE64_SEG  equ gdt_code64 - gdt_start

; ===========================================================================
; 32-bit protected mode entry.
;
;   PML4[0] -> PDPT
;   PDPT[0] -> PD
;   PD[0..511] -> 2 MiB physical pages
;
; Then enable PAE, load CR3, set EFER.LME, enable paging,
; and far jump to 64-bit mode.
; ===========================================================================
BITS 32
pm32_entry:
            mov     ax, DATA_SEG
            mov     ds, ax
            mov     es, ax
            mov     fs, ax
            mov     gs, ax
            mov     ss, ax
            mov     esp, STACK_TOP
            mov     ebp, esp
            cld

            ; Clear PML4, PDPT and PD.
            mov     edi, PT_BASE
            xor     eax, eax
            mov     ecx, 0x3000 / 4
            rep     stosd

            ; PML4[0] = PDPT physical address | PRESENT | WRITABLE
            mov     dword [PML4], PDPT + 3
            mov     dword [PML4 + 4], 0

            ; PDPT[0] = PD physical address | PRESENT | WRITABLE
            mov     dword [PDPT], PD + 3
            mov     dword [PDPT + 4], 0

            ; Fill PD with 512 x 2 MiB pages.
            ; Entry flags: PRESENT | WRITABLE | PAGE_SIZE = 0x83
            mov     edi, PD
            mov     eax, 0x00000083
            mov     ecx, 512
.fill:
            mov     [edi], eax
            add     eax, 0x00200000
            add     edi, 8
            loop    .fill

            ; Enable PAE.
            mov     eax, cr4
            or      eax, 0x20               ; CR4.PAE
            mov     cr4, eax

            ; Load page table base.
            mov     eax, PML4
            mov     cr3, eax

            ; Enable long mode via EFER.LME.
            mov     ecx, 0xC0000080         ; IA32_EFER
            rdmsr
            or      eax, 0x100              ; EFER.LME
            wrmsr

            ; Enable paging. Protected mode is already enabled.
            mov     eax, cr0
            or      eax, 0x80000000         ; CR0.PG
            mov     cr0, eax

            ; Now enter true 64-bit mode.
            jmp     CODE64_SEG:long_entry

; ===========================================================================
; 64-bit long mode entry.
;
; The CPU is now in long mode with identity-mapped first 1 GiB.
; The kernel must be 64-bit and linked at KERNEL_OFF.
; ===========================================================================
BITS 64
long_entry:
            mov     ax, DATA_SEG
            mov     ds, ax
            mov     es, ax
            mov     fs, ax
            mov     gs, ax
            mov     ss, ax

            mov     rsp, STACK_TOP
            mov     rbp, rsp
            cld

            mov     eax, KERNEL_OFF
            call    rax

.hang:
            cli
            hlt
            jmp     .hang

; ===========================================================================
; Data
; ===========================================================================
BITS 16

boot_drive  db 0
spt         dw 18
heads       dw 2
cur_lba     dw 0
left        dw 0
dst_seg     dw 0
chs_cx      dw 0
chs_dh      db 0
count       db 0
use_lba     db 0
dap         times 16 db 0       ; INT 13h AH=42h disk address packet

msg_hello   db "EOS s2: loading kernel ", 0
msg_crlf    db 13, 10, 0
msg_pm      db "entering long mode", 13, 10, 0
msg_disk    db 13, 10, "S2: disk read failed", 13, 10, 0
msg_a20     db 13, 10, "S2: A20 could not be enabled", 13, 10, 0

            times (4 * 512) - ($ - $$) db 0     ; pad to exactly 4 sectors