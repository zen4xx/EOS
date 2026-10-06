#include "power.h"
#include "stdint.h"

void shutdown(void) {
    register uint64_t num __asm__("rax") = 100;
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        :
        : "memory", "rcx", "r11"
    );
}

void reboot(void) {
    register uint64_t num __asm__("rax") = 101;
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        :
        : "memory", "rcx", "r11"
    );
}