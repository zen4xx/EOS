/* EOS kernel layout. Keeps everything packed so kernel.bin stays small,
   and exports __bss_start/__bss_end so the entry stub can clear .bss. */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x1000;

    .text : {
        *(.text.boot)          /* _start must be the very first byte */
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    . = ALIGN(4);
    __bss_start = .;
    .bss : {
        *(COMMON)
        *(.bss*)
    }
    . = ALIGN(4);
    __bss_end = .;

    _kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.debug*)
    }
}
