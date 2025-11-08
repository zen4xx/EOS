#include "power.h"

void shutdown() {
    __asm__ ("movl $100, %eax"); // 100 is syscall num
    __asm__ __volatile__ ("int $0x80"); //syscall
}

void reboot() {
    __asm__ ("movl $101, %eax"); // 101 is syscall num
    __asm__ __volatile__ ("int $0x80"); //syscall
}