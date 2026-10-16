#ifndef PMM_H
#define PMM_H

#include "../libc/stdint.h"

void pmm_init(uint64_t base_address, uint64_t size);
uint64_t pmm_alloc(uint64_t size, uint64_t alignment);
void pmm_free(uint64_t phys_addr, uint64_t size);

#endif