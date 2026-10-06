#ifndef ALLOC_H
#define ALLOC_H

#include "../cpu/types.h"
#include "../libc/memory.h"
#include "../libc/error.h"

typedef struct Block {
    size_t size;
    u8     is_free;        
    struct Block* next;    
    struct Block* prev;    
} __attribute__((aligned(16))) Block;

void   init_allocator(void);
void*  allocate(size_t size);
void   release(void* ptr);
size_t get_total_allocated_size(void);

#endif /* ALLOC_H */