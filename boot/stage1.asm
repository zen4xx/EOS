; ===========================================================================
; EOS - stage 1 boot sector (LBA 0, exactly 512 bytes)
;
; Job: get into a *known* CPU state and load stage 2. Nothing else.
; Everything here is deliberately geometry-independent: stage 2 lives on
; cylinder 0 / head 0 / sectors 2..5, which exists on every floppy-like
; geometry a BIOS can possibly report.
; ===========================================================================
BITS 16
ORG 0x7C00

STAGE2_SEG      equ 0x0000
STAGE2_OFF      equ 0x7E00
STAGE2_SECTOR   equ 2               ; CHS sector number (1-based) = LBA 1
STAGE2_SECTORS  equ 4               ; 4 sectors = 2 KiB of stage 2

            jmp short _start
            nop

; --------------------------------------------------------------------------
; BIOS Parameter Block.
; A USB floppy adapter is presented to the BIOS through its USB-FDD emulation
; layer, and a lot of firmware refuses to boot - or picks the wrong emulation
; type (ZIP/HDD instead of FDD) - if there is no plausible BPB here.
; QEMU never cared, which is why this worked in the emulator.
; --------------------------------------------------------------------------
bpb_oem                 db "EOS  1.0"
bpb_bytes_per_sector    dw 512
bpb_sectors_per_cluster db 1
bpb_reserved_sectors    dw 1
bpb_num_fats            db 2
bpb_root_entries        dw 224
bpb_total_sectors       dw 2880
bpb_media               db 0xF0     ; 3.5" 1.44 MB
bpb_sectors_per_fat     dw 9
bpb_sectors_per_track   dw 18
bpb_heads               dw 2
bpb_hidden_sectors      dd 0
bpb_large_sectors       dd 0
bpb_drive_number        db 0x00
bpb_reserved            db 0
bpb_signature           db 0x29
bpb_volume_id           dd 0x454F5301
bpb_volume_label        db "EOS BOOT   "
bpb_fs_type             db "FAT12   "

; --------------------------------------------------------------------------
_start:
            cli
            xor     ax, ax
            mov     ds, ax                  ; <-- the original code never did
            mov     es, ax                  ;     this. QEMU happened to hand
            mov     ss, ax                  ;     us zeroed segments, real
            mov     sp, 0x7C00              ;     firmware does not.
            sti
            jmp     0x0000:.canonical       ; some BIOSes enter at 07C0:0000

.canonical:
            mov     [boot_drive], dl        ; DL = BIOS drive we booted from

            mov     si, msg_s1
            call    puts

            ; ---- load stage 2 -------------------------------------------
            mov     di, 5                   ; retry counter
.read:
            mov     ax, STAGE2_SEG
            mov     es, ax
            mov     bx, STAGE2_OFF
            mov     ah, 0x02
            mov     al, STAGE2_SECTORS
            mov     ch, 0                   ; cylinder 0
            mov     cl, STAGE2_SECTOR       ; sector 2
            mov     dh, 0                   ; head 0
            mov     dl, [boot_drive]
            int     0x13
            jnc     .ok

            xor     ah, ah                  ; reset controller and try again;
            mov     dl, [boot_drive]        ; real drives fail the first read
            int     0x13                    ; far more often than you'd think
            dec     di
            jnz     .read

            mov     si, msg_err
            call    puts
.halt:      cli
            hlt
            jmp     .halt

.ok:
            mov     dl, [boot_drive]        ; hand the drive number to stage 2
            jmp     STAGE2_SEG:STAGE2_OFF

; --------------------------------------------------------------------------
; puts - DS:SI = NUL terminated string
puts:
            pusha
            mov     ah, 0x0E
            xor     bx, bx
.loop:      lodsb
            test    al, al
            jz      .done
            int     0x10
            jmp     .loop
.done:      popa
            ret

boot_drive  db 0
msg_s1      db "EOS s1", 13, 10, 0
msg_err     db "S1: disk error", 13, 10, 0

            times 510 - ($ - $$) db 0
            dw 0xAA55
