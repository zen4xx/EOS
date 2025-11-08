#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "../kernel/kernel_api.h"
#include "../kernel/alloc.h"
#include "syscall.h"

#define ACPI_PORT_1 0x604
#define ACPI_PORT_2 0xb004

u32 syscall_handler(
    u32 num, // EAX
    u32 a1,  // EBX
    u32 a2,  // ECX
    u32 a3,  // EDX
    u32 a4,  // ESI
    u32 a5,  // EDI
    u32 a6   // EBP
)
{
    switch (num)
    {
    case SYSCALL_PRINT_STRING:
        krnl_print((char*)a1);
        return 0;

    case SYSCALL_GETCHAR:
        asm volatile("sti"); //enable interuption
        while(_current_char == '\0'); 
        asm volatile("cli");
        return _current_char;

    case SYSCALL_MALLOC:
        return (u32)allocate(a1);

    case SYSCALL_FREE:
        release((void*)a1);
        return 0;

    case SYSCALL_REALLOC:
        release((void*)a1);
        return (u32)allocate(a2);

    case SYSCALL_MALLOC_STATS:
        return get_total_allocated_size();

    case SYSCALL_CLEAR:
        clear();
        return 0;

    case SYSCALL_POWER_OFF:
        port_word_out(ACPI_PORT_1, 0x2000); 
        port_word_out(ACPI_PORT_2, 0x2000); // if pc is still alive
        asm volatile("hlt"); // as a last resort, halt the cpu
        return 0;

    case SYSCALL_REBOOT:
        port_byte_out(0x64, 0xFE);
        return 0;

    default:
        return ((u32)-1);

    }   

    return 0;
}
