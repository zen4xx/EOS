#include "pmm.h"
#include "../libc/memory.h"

#define PAGE_SIZE 4096
#define MAX_PAGES 65536 // 256 mb

static uint8_t pmm_bitmap[MAX_PAGES / 8];
static uint64_t pmm_base_phys;

void pmm_init(uint64_t base_address, uint64_t size) {
    pmm_base_phys = (base_address + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    memset(pmm_bitmap, 0, sizeof(pmm_bitmap));
    
    uint64_t num_pages = size / PAGE_SIZE;
    if (num_pages > MAX_PAGES) num_pages = MAX_PAGES;
    
    for (int i = 0; i < 16; i++) {
        pmm_bitmap[i / 8] |= (1 << (i % 8));
    }
}

uint64_t pmm_alloc(uint64_t size, uint64_t alignment) {
    uint64_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t align_pages = alignment / PAGE_SIZE;
    if (align_pages == 0) align_pages = 1;
    
    for (uint64_t start = 0; start + pages_needed <= MAX_PAGES; start += align_pages) {
        int free = 1;
        for (uint64_t i = 0; i < pages_needed; i++) {
            if (pmm_bitmap[(start + i) / 8] & (1 << ((start + i) % 8))) {
                free = 0;
                break;
            }
        }
        
        if (free) {
            for (uint64_t i = 0; i < pages_needed; i++) {
                pmm_bitmap[(start + i) / 8] |= (1 << ((start + i) % 8));
            }
            return pmm_base_phys + start * PAGE_SIZE;
        }
    }
    
    return 0;
}

void pmm_free(uint64_t phys_addr, uint64_t size) {
    uint64_t page_index = (phys_addr - pmm_base_phys) / PAGE_SIZE;
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t i = 0; i < pages; i++) {
        pmm_bitmap[(page_index + i) / 8] &= ~(1 << ((page_index + i) % 8));
    }
}