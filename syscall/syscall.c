#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../libc/alloc.h"
#include "../kernel/kernel_api.h"
#include "syscall.h"

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

    default:
        return ((u32)-1);

    }   

    return 0;
}
