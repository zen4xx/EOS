#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "../kernel/kernel_api.h"
#include "../kernel/alloc.h"
#include "syscall.h"

#define ACPI_PORT_1 0x604
#define ACPI_PORT_2 0xb004

/*
 * Register indices in the saved frame.
 * The order matches the push sequence in isr80:
 *   rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15
 *
 * On the stack (top to bottom): r15, r14, ..., rax
 * So regs[0] = r15, regs[14] = rax
 */
#define REG_RAX 14
#define REG_RBX 13
#define REG_RCX 12
#define REG_RDX 11
#define REG_RSI 10
#define REG_RDI 9
#define REG_RBP 8
#define REG_R8  7
#define REG_R9  6
#define REG_R10 5
#define REG_R11 4
#define REG_R12 3
#define REG_R13 2
#define REG_R14 1
#define REG_R15 0

u64 syscall_handler(u64* regs) {
    u64 num = regs[REG_RAX];  // Syscall number
    u64 a1  = regs[REG_RDI];  // First argument
    u64 a2  = regs[REG_RSI];  // Second argument
    u64 a3  = regs[REG_RDX];  // Third argument
    u64 a4  = regs[REG_RCX];  // Fourth argument
    u64 a5  = regs[REG_R8];   // Fifth argument

    switch (num) {
    case SYSCALL_PRINT_STRING:
        krnl_print((char*)a1);
        return 0;

    case SYSCALL_GETCHAR:
        _current_char = '\0';
        asm volatile("sti");
        while (_current_char == '\0');
        asm volatile("cli");
        return (u64)_current_char;

    case SYSCALL_MALLOC:
        return (u64)allocate((size_t)a1);

    case SYSCALL_FREE:
        release((void*)a1);
        return 0;

    case SYSCALL_REALLOC:
        release((void*)a1);
        return (u64)allocate((size_t)a2);

    case SYSCALL_MALLOC_STATS:
        return (u64)get_total_allocated_size();

    case SYSCALL_CLEAR:
        clear();
        return 0;

    case SYSCALL_POWER_OFF:
        port_word_out(ACPI_PORT_1, 0x2000);
        port_word_out(ACPI_PORT_2, 0x2000);
        asm volatile("hlt");
        return 0;

    case SYSCALL_REBOOT:
        port_byte_out(0x64, 0xFE);
        return 0;

    default:
        return (u64)-1;
    }
}