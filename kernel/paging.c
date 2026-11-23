#include "paging.h"
#include "pmm.h"
#include "../libc/error.h"

#define ADDR_MASK 0x000FFFFFFFFFF000ULL   /* physical address bits 12..51 */
#define FLAG_MASK 0xFFF0000000000FFFULL   /* low 12 flag bits + NX etc.   */
#define PAGE_PS   (1ULL << 7)             /* large page in PD/PDPT        */
#define LARGE_2M  0x200000ULL
#define LARGE_1G  0x40000000ULL

#define EFER_MSR  0xC0000080u
#define EFER_NXE  (1u << 11)

static uint64_t pml4_phys;

static inline uint64_t read_cr3(void) {
    uint64_t v;
    asm volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}
static inline void write_cr3(uint64_t v) {
    asm volatile("mov %0, %%cr3" :: "r"(v) : "memory");
}
static inline void invlpg(uint64_t v) {
    asm volatile("invlpg (%0)" :: "r"(v) : "memory");
}

/* Page-table pages come from the PMM pool inside the identity map,
 * so physical == virtual. If the kernel ever moves to a higher-half
 * layout, this is the ONLY place that needs a phys->virt fix. */
static inline uint64_t *table_at(uint64_t entry) {
    return (uint64_t *)(entry & ADDR_MASK);
}

void paging_init(void) {
    pml4_phys = read_cr3() & ADDR_MASK;

    /* Allow PAGE_NX. NXE is architectural in long mode. */
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(EFER_MSR));
    lo |= EFER_NXE;
    asm volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(EFER_MSR));
}

uint64_t paging_cr3(void) { return pml4_phys; }

/* Return the next-level table for *entry, allocating a zeroed page from
 * the PMM if it is not present. Intermediate entries are permissive;
 * the final PTE carries the real permissions. */
static uint64_t *next_table(uint64_t *entry) {
    if (*entry & PAGE_PRESENT) return table_at(*entry);
    uint64_t p = pmm_alloc_page_zero();
    if (!p) return 0;
    *entry = p | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    return (uint64_t *)p;
}

/* Split a 2 MiB identity page into 512 4 KiB pages with the same
 * backing memory and attributes, then point the PDE at the new PT. */
static int split_large_page(uint64_t *pde) {
    uint64_t old  = *pde;
    uint64_t base = old & ADDR_MASK & ~(LARGE_2M - 1);
    uint64_t keep = old & (PAGE_WRITE | PAGE_USER | PAGE_PWT | PAGE_PCD | PAGE_NX);
    /* PAT moves from bit 12 (2 MiB page) to bit 7 (4 KiB page). */
    uint64_t pat  = (old & (1ULL << 12)) ? (1ULL << 7) : 0;

    uint64_t p = pmm_alloc_page_zero();
    if (!p) return -1;
    uint64_t *pt = (uint64_t *)p;
    for (uint64_t i = 0; i < 512; i++) {
        pt[i] = (base + i * PMM_PAGE_SIZE) | keep | pat | PAGE_PRESENT;
    }
    *pde = p | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    /* Flush stale 2 MiB TLB entries (invlpg alone is not guaranteed
     * to invalidate large-page translations on every CPU). */
    write_cr3(pml4_phys);
    return 0;
}

int paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    if ((virt & (PMM_PAGE_SIZE - 1)) || (phys & (PMM_PAGE_SIZE - 1))) return -1;
    if (!pml4_phys) paging_init();

    uint64_t *pml4 = (uint64_t *)pml4_phys;
    uint64_t *pdpt = next_table(&pml4[(virt >> 39) & 511]);
    if (!pdpt) return -1;

    uint64_t *pdpe = &pdpt[(virt >> 30) & 511];
    if ((*pdpe & PAGE_PRESENT) && (*pdpe & PAGE_PS)) {
        err("paging: 1 GiB page split not supported\n");
        return -1;   /* stage2 never creates these */
    }
    uint64_t *pd = next_table(pdpe);
    if (!pd) return -1;

    uint64_t *pde = &pd[(virt >> 21) & 511];
    if ((*pde & PAGE_PRESENT) && (*pde & PAGE_PS)) {
        if (split_large_page(pde) != 0) return -1;
    }
    uint64_t *pt = next_table(pde);
    if (!pt) return -1;

    pt[(virt >> 12) & 511] = (phys & ADDR_MASK) | (flags & FLAG_MASK) | PAGE_PRESENT;
    invlpg(virt);
    return 0;
}

void paging_unmap_page(uint64_t virt) {
    if (!pml4_phys) return;
    uint64_t *pml4 = (uint64_t *)pml4_phys;
    uint64_t e = pml4[(virt >> 39) & 511];
    if (!(e & PAGE_PRESENT)) return;
    uint64_t *pdpt = table_at(e);
    e = pdpt[(virt >> 30) & 511];
    if (!(e & PAGE_PRESENT) || (e & PAGE_PS)) return;
    uint64_t *pd = table_at(e);
    e = pd[(virt >> 21) & 511];
    if (!(e & PAGE_PRESENT)) return;
    if (e & PAGE_PS) {
        err("paging: cannot unmap 4 KiB of a 2 MiB page\n");
        return;
    }
    uint64_t *pt = table_at(e);
    pt[(virt >> 12) & 511] = 0;
    invlpg(virt);
}

uint64_t paging_virt_to_phys(uint64_t virt) {
    if (!pml4_phys) return 0;
 
    uint64_t *pml4 = (uint64_t *)pml4_phys;
    uint64_t e = pml4[(virt >> 39) & 511];
    if (!(e & PAGE_PRESENT)) return 0;
    
    uint64_t *pdpt = table_at(e);
    e = pdpt[(virt >> 30) & 511];
    if (!(e & PAGE_PRESENT)) return 0;
    if (e & PAGE_PS) return (e & ADDR_MASK & ~(LARGE_1G - 1)) + (virt & (LARGE_1G - 1));
    
    uint64_t *pd = table_at(e);
    e = pd[(virt >> 21) & 511];
    if (!(e & PAGE_PRESENT)) return 0;
    if (e & PAGE_PS) return (e & ADDR_MASK & ~(LARGE_2M - 1)) + (virt & (LARGE_2M - 1));
    
    uint64_t *pt = table_at(e);
    e = pt[(virt >> 12) & 511];
    if (!(e & PAGE_PRESENT)) return 0;
    return (e & ADDR_MASK) + (virt & (PMM_PAGE_SIZE - 1));
}

int paging_alloc_map(uint64_t virt, uint64_t pages, uint64_t flags) {
    if (virt & (PMM_PAGE_SIZE - 1)) return -1;
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t p = pmm_alloc_page();
        if (!p || paging_map_page(virt + i * PMM_PAGE_SIZE, p, flags) != 0) {
            if (p) pmm_free_page(p);
            paging_free_unmap(virt, i);   /* roll back */
            return -1;
        }
    }
    return 0;
}

void paging_free_unmap(uint64_t virt, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t v = virt + i * PMM_PAGE_SIZE;
        uint64_t p = paging_virt_to_phys(v);
        paging_unmap_page(v);
        if (p) pmm_free_page(p);
    }
}

int paging_identity_extend(uint64_t phys_end) {
    uint64_t *pml4 = (uint64_t*)pml4_phys;
    uint64_t *pdpt = table_at(pml4[0]);            // boot-PDPT, 512 slots
    uint64_t gigs  = (phys_end + LARGE_1G - 1) / LARGE_1G;

    for (uint64_t g = 4; g < gigs; g++) {          // [0..4 GiB) allready mapped stage2
        if (pdpt[g] & PAGE_PRESENT) continue;
        uint64_t pd = pmm_alloc_page_zero();
        if (!pd) return -1;
        uint64_t *p = (uint64_t*)pd;
        for (int i = 0; i < 512; i++)
            p[i] = (g * LARGE_1G + i * LARGE_2M) | PAGE_PRESENT | PAGE_WRITE | PAGE_PS;
        pdpt[g] = pd | PAGE_PRESENT | PAGE_WRITE;
    }
    write_cr3(pml4_phys);                          // flush TLB
    return 0;
}