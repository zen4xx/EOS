#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../libc/alloc.h"
#include "../libc/string.h"
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
        while(_current_char == '\0'); 
        return (u32)_current_char;

    default:
        return ((u32)-1);

    }   

    return 0;
}
