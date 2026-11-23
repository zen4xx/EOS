#ifndef PAGING_H
#define PAGING_H

#include "../libc/stdint.h"

/* x86_64 4-level paging on top of the PMM.
 *
 * stage2 already identity-mapped the first 4 GiB with 2 MiB pages and
 * loaded CR3. paging_init() adopts that PML4 - it does NOT build a new
 * address space. 2 MiB identity pages are split into 4 KiB pages on
 * demand, so remapping a page never loses the surrounding mapping.
 *
 * Callers must serialize access (the heap allocator does this with
 * irq_save/irq_restore). Single core, so no TLB shootdown is needed. */

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITE   (1ULL << 1)
#define PAGE_USER    (1ULL << 2)
#define PAGE_PWT     (1ULL << 3)
#define PAGE_PCD     (1ULL << 4)
#define PAGE_NX      (1ULL << 63)   /* paging_init() enables EFER.NXE */

void     paging_init(void);

/* Map one 4 KiB page. virt/phys must be page-aligned. 0 on success. */
int      paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void     paging_unmap_page(uint64_t virt);

/* Walk the current tables. Returns 0 if unmapped. Handles 2 MiB/1 GiB. */
uint64_t paging_virt_to_phys(uint64_t virt);

/* Allocate `pages` physical pages (may be scattered) and map them to a
 * contiguous virtual range. Rolls back on OOM. 0 on success, -1 on OOM. */
int      paging_alloc_map(uint64_t virt, uint64_t pages, uint64_t flags);

/* Unmap the range and return every backing page to the PMM. */
void     paging_free_unmap(uint64_t virt, uint64_t pages);

uint64_t paging_cr3(void);

int paging_identity_extend(uint64_t phys_end);

#endif /* PAGING_H */
