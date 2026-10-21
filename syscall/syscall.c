#include "syscall.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "../kernel/kernel_api.h"
#include "../kernel/alloc.h"
#include "../multitasking/mt.h"
#include "../fs/fd.h"
#include "../fs/ext2.h"

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
//    u64 a4  = regs[REG_RCX];  // Fourth argument (uncomment if needed)
//    u64 a5  = regs[REG_R8];   // Fifth argument  (uncomment if needed)

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

    case SYSCALL_EXIT:
        task_exit();
        return 0;

    case SYSCALL_MALLOC:
        return (u64)allocate((size_t)a1);

    case SYSCALL_FREE:
        release((void*)a1);
        return 0;

    case SYSCALL_FOPEN:
        return (u64)fd_open(get_current_fd_table(), (const char*)a1, (uint32_t) a2);
    
    case SYSCALL_FCLOSE:
        return (u64)fd_close(get_current_fd_table(), (int)a1);

    case SYSCALL_FREAD:
        return (u64)fd_read(get_current_fd_table(), (int)a1, (void*)a2, (size_t)a3);

    case SYSCALL_FWRITE:
        return (u64)fd_write(get_current_fd_table(), (int)a1, (const void*)a2, (size_t)a3);

    case SYSCALL_MKDIR:
        return (u64)vfs_mkdir((const char*)a1);    

    case SYSCALL_READDIR:
        return (u64)vfs_readdir((const char*)a1, (uint32_t)a2, (vfs_dirent_t*)a3);

    case SYSCALL_GET_CWD:
        return (u64)vfs_get_cwd((char*)a1, (uint32_t)a2);
    
    case SYSCALL_CHANGE_CWD:
        return (u64)vfs_change_cwd((const char*)a1);

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
