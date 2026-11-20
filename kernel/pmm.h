#ifndef PMM_H
#define PMM_H

#include "../libc/stdint.h"

#define PMM_PAGE_SIZE 4096ULL

void     pmm_init(uint64_t base_address, uint64_t size);
void     pmm_init_region(uint64_t base_address, uint64_t size);
void     pmm_reserve(uint64_t base_address, uint64_t size);

uint64_t pmm_alloc_page(void);                 // one 4 KiB page
uint64_t pmm_alloc_page_zero(void);            // one 4 KiB page, zeroed
void     pmm_free_page(uint64_t phys_addr);
uint64_t pmm_alloc_pages(uint64_t count);      // physically contiguous run
uint64_t pmm_alloc_pages_aligned(uint64_t count, uint64_t alignment);
void     pmm_free_pages(uint64_t phys_addr, uint64_t count);

// legacy for ahci driver
uint64_t pmm_alloc(uint64_t size, uint64_t alignment);
void     pmm_free(uint64_t phys_addr, uint64_t size);

uint64_t pmm_total_pages(void);
uint64_t pmm_used_pages(void);
uint64_t pmm_free_pages_count(void);

#endif /* PMM_H */
