/* EOS kernel layout
 *
 * The kernel is linked at physical/virtual 0x1000.
 * stage2 identity-maps the first 1 GiB, so 0x1000 is directly usable.
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    text   PT_LOAD FLAGS(5);  /* R-X */
    rodata PT_LOAD FLAGS(4);  /* R-- */
    data   PT_LOAD FLAGS(6);  /* RW- */
}

SECTIONS
{
    . = 0x1000;

    .text : {
        KEEP(*(.text.boot))      /* _start must be the very first byte */
        *(.text .text.*)
    } :text

    . = ALIGN(16);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(16);

    .data : {
        *(.data .data.*)
    } :data

    . = ALIGN(16);
    . = MAX(., 0x100000);

    __bss_start = .;

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    . = ALIGN(16);
    __bss_end = .;

    . = ALIGN(16);
    _kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.debug*)
    }
}