#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../libc/alloc.h"
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
    krnl_print("syscall\n");
    /*
    switch (num)
    {
    case SYSCALL_PRINT_CHAR:
        krnl_print((char*)a1);
        return 0;
    case SYSCALL_PRINT_STRING:
        krnl_print((char*)a1);
        return 0;

    default:
        return ((u32)-1);

    }   
    */

    return 0;
}
