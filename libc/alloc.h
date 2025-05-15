#ifndef ALLOC_H
#define ALLOC_H
#include "../cpu/types.h"

void* allocate(u32 size);
void release(void* ptr);

#endif