#ifndef STDLIB_H
#define STDLIB_H

#include "../cpu/types.h"
#include "stddef.h"

void* malloc(u32 size);
void free(void* ptr);
void* realloc(void* ptr, u32 size);
u32 malloc_info(); // returns a total allocated size in bytes

#endif
