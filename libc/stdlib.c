#include "stdlib.h"

void *malloc(uint64_t size) {
    register uint64_t num __asm__("rax") = 10;
    register uint64_t arg1 __asm__("rdi") = size;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1)
        : "memory", "rcx", "r11"
    );
    
    return (void *)num;
}

uint64_t malloc_info(void) {
    register uint64_t num __asm__("rax") = 13;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        :
        : "memory", "rcx", "r11"
    );
    
    return num;
}

void free(void *ptr) {
    register uint64_t num __asm__("rax") = 12;
    register uint64_t arg1 __asm__("rdi") = (uint64_t)ptr;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1)
        : "memory", "rcx", "r11"
    );
}

void *realloc(void *ptr, uint64_t size) {
    register uint64_t num __asm__("rax") = 11;
    register uint64_t arg1 __asm__("rdi") = (uint64_t)ptr;
    register uint64_t arg2 __asm__("rsi") = size;
    
    __asm__ __volatile__ (
        "int $0x80"
        : "+r" (num)
        : "r" (arg1), "r" (arg2)
        : "memory", "rcx", "r11"
    );
    
    return (void *)num;
}

int fopen(const char *path, uint32_t flags)
{
    register uint64_t rax __asm__("rax") = 4;
    register uint64_t rdi __asm__("rdi") = (uint64_t)path;
    register uint64_t rsi __asm__("rsi") = (uint64_t)flags;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi)
        : "memory"
    );

    return (int)rax;
}

int fclose(int fd)
{
    register uint64_t rax __asm__("rax") = 5;
    register uint64_t rdi __asm__("rdi") = (uint64_t)fd;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi)
        : "memory"
    );

    return (int)rax;
}

int fread(int fd, void *buffer, size_t size)
{
    register uint64_t rax __asm__("rax") = 6;
    register uint64_t rdi __asm__("rdi") = (uint64_t)fd;
    register uint64_t rsi __asm__("rsi") = (uint64_t)buffer;
    register uint64_t rdx __asm__("rdx") = (uint64_t)size;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx)
        : "memory"
    );

    return (int)rax;
}

int fwrite(int fd, const void *buffer, size_t size)
{
    register uint64_t rax __asm__("rax") = 7;
    register uint64_t rdi __asm__("rdi") = (uint64_t)fd;
    register uint64_t rsi __asm__("rsi") = (uint64_t)buffer;
    register uint64_t rdx __asm__("rdx") = (uint64_t)size;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx)
        : "memory"
    );

    return (int)rax;
}

int mkdir(const char *path)
{
    register uint64_t rax __asm__("rax") = 8;
    register uint64_t rdi __asm__("rdi") = (uint64_t)path;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi)
        : "memory"
    );

    return (int)rax;
}

int readdir(const char *path, uint32_t index, dirent_t *entry)
{
    register uint64_t rax __asm__("rax") = 9;
    register uint64_t rdi __asm__("rdi") = (uint64_t)path;
    register uint64_t rsi __asm__("rsi") = (uint64_t)index;
    register uint64_t rdx __asm__("rdx") = (uint64_t)entry;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx)
        : "memory"
    );

    return (int)rax;
}

int get_cwd(char *buffer, uint32_t size)
{
    register uint64_t rax __asm__("rax") = 14;
    register uint64_t rdi __asm__("rdi") = (uint64_t)buffer;
    register uint64_t rsi __asm__("rsi") = (uint64_t)size;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi)
        : "memory"
    );

    return (int)rax;
}

int change_cwd(const char *path)
{
    register uint64_t rax __asm__("rax") = 15;
    register uint64_t rdi __asm__("rdi") = (uint64_t)path;

    __asm__ volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi)
        : "memory"
    );

    return (int)rax;
}