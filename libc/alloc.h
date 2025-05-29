#ifndef ALLOC_H
#define ALLOC_H
#include "../cpu/types.h"
#include "../libc/error.h"

void* allocate(u32 size);
void release(void* ptr);
u32 get_allocated_size();

#endif