#ifndef STDLIB_H
#define STDLIB_H

#include "../cpu/types.h"
#include "stddef.h"

void* malloc(u64 size);
void free(void* ptr);
void* realloc(void* ptr, u64 size);
u64 malloc_info(void); // return a total allocated size in bytes

#endif
