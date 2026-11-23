#include "pmm.h"
#include "irqflags.h"
#include "../libc/memory.h"
#include "../libc/error.h"

#define PMM_MAX_PAGES (1ULL << 17)
#define PMM_WORDS     (PMM_MAX_PAGES / 64)
#define PAGE PMM_PAGE_SIZE

static uint64_t pmm_base_phys;           /* page-aligned          */
static uint64_t pmm_pages;               /* actually managed      */
static uint64_t pmm_used;
static uint64_t pmm_hint;                /* first-fit cursor      */

static uint64_t* pmm_bitmap;
static uint64_t  pmm_bm_pages;

/* ---------------------------------------------------------------- bits */

static inline int bit_test(uint64_t i)  { return (int)((pmm_bitmap[i >> 6] >> (i & 63)) & 1); }
static inline void bit_set(uint64_t i)  { pmm_bitmap[i >> 6] |=  (1ULL << (i & 63)); }
static inline void bit_clear(uint64_t i){ pmm_bitmap[i >> 6] &= ~(1ULL << (i & 63)); }

/* Is [start, start+count) free? Whole words are checked at once. */
static int range_is_free(uint64_t start, uint64_t count) {
    uint64_t end = start + count, i = start;
    while (i < end && (i & 63)) { if (bit_test(i)) return 0; i++; }
    while (i + 64 <= end)       { if (pmm_bitmap[i >> 6]) return 0; i += 64; }
    while (i < end)             { if (bit_test(i)) return 0; i++; }
    return 1;
}

static void range_set(uint64_t start, uint64_t count) {
    uint64_t end = start + count, i = start;
    while (i < end && (i & 63)) { bit_set(i); i++; }
    while (i + 64 <= end)       { pmm_bitmap[i >> 6] = ~0ULL; i += 64; }
    while (i < end)             { bit_set(i); i++; }
}

/* ------------------------------------------------------- lockless core */

static uint64_t alloc_run(uint64_t count, uint64_t align_bytes) {
    if (count == 0 || count > pmm_pages) return 0;

    uint64_t ap = (align_bytes + PAGE - 1) / PAGE;
    if (ap == 0) ap = 1;
    if (ap & (ap - 1)) {                    /* round alignment up to pow2 */
        uint64_t p = 1;
        while (p < ap) p <<= 1;
        ap = p;
    }

    /* Align in ABSOLUTE physical space, not relative to the pool base. */
    uint64_t base_idx = pmm_base_phys / PAGE;
    uint64_t start = (ap - (base_idx % ap)) % ap;

    while (start + count <= pmm_pages) {
        /* Fast skip of fully used words (only valid for ap == 1). */
        if (ap == 1 && !(start & 63) && pmm_bitmap[start >> 6] == ~0ULL) {
            start += 64;
            continue;
        }
        if (range_is_free(start, count)) {
            range_set(start, count);
            pmm_used += count;
            pmm_hint = start + count;
            return pmm_base_phys + start * PAGE;
        }
        start += ap;
    }
    return 0;
}

static uint64_t alloc_one(void) {
    /* Single-page fast path with a rotating hint. */
    for (uint64_t pass = 0; pass < 2; pass++) {
        uint64_t from = pass == 0 ? pmm_hint : 0;
        uint64_t to   = pass == 0 ? pmm_pages : pmm_hint;
        uint64_t i = from;
        while (i < to) {
            uint64_t w = i >> 6;
            uint64_t free_bits = ~pmm_bitmap[w];
            if (i == from) free_bits &= ~0ULL << (from & 63);
            if (free_bits) {
                uint64_t bit = (uint64_t)__builtin_ctzll(free_bits);
                uint64_t idx = w * 64 + bit;
                if (idx >= from && idx < to && idx < pmm_pages) {
                    bit_set(idx);
                    pmm_used++;
                    pmm_hint = idx + 1;
                    return pmm_base_phys + idx * PAGE;
                }
            }
            i = (w + 1) * 64;
        }
    }
    return alloc_run(1, PAGE);
}

static void free_run(uint64_t phys, uint64_t count) {
    if (count == 0) return;
    if (phys < pmm_base_phys || (phys & (PAGE - 1))) {
        err("pmm: free of unmanaged/unaligned address\n");
        return;
    }
    uint64_t idx = (phys - pmm_base_phys) / PAGE;
    if (idx + count > pmm_pages) {
        err("pmm: free out of range\n");
        return;
    }
    uint64_t cleared = 0;
    for (uint64_t i = idx; i < idx + count; i++) {
        if (bit_test(i)) { bit_clear(i); cleared++; }
    }
    if (cleared != count) err("pmm: double free detected\n");
    pmm_used -= cleared;
    if (idx < pmm_hint) pmm_hint = idx;
}

// ---------------------public api----------------------------------

void pmm_init(uint64_t base_address, uint64_t size) {
    uint64_t f = irq_save();
    uint64_t base  = (base_address + PAGE - 1) & ~(PAGE - 1);
    uint64_t end   = (base_address + size) & ~(PAGE - 1);
    uint64_t pages = end > base ? (end - base) / PAGE : 0;

    uint64_t words = (pages + 63) / 64;
    pmm_bm_pages   = (words * 8 + PAGE - 1) / PAGE;

    pmm_bitmap    = (uint64_t*)base;   // identity: phys == virt, pool < 4 GiB
    pmm_base_phys = base;
    pmm_pages     = pages;
    pmm_used      = pages;
    pmm_hint      = 0;
    memset(pmm_bitmap, 0xFF, words * 8);
    irq_restore(f);
}

void pmm_init_region(uint64_t base_address, uint64_t size) {
    uint64_t f = irq_save();
    uint64_t pool_end = pmm_base_phys + pmm_pages * PAGE;
    uint64_t lo = (base_address + PAGE - 1) & ~(PAGE - 1);
    uint64_t hi = (base_address + size) & ~(PAGE - 1);
    
    uint64_t bm_end = pmm_base_phys + pmm_bm_pages * PAGE;
    if (lo < bm_end) lo = bm_end;

    if (lo < pmm_base_phys) lo = pmm_base_phys;
    if (hi > pool_end)      hi = pool_end;
    for (uint64_t a = lo; a < hi; a += PAGE) {
        uint64_t i = (a - pmm_base_phys) / PAGE;
        if (bit_test(i)) { bit_clear(i); pmm_used--; }
    }
    irq_restore(f);
}

void pmm_reserve(uint64_t base_address, uint64_t size) {
    uint64_t f = irq_save();
    uint64_t pool_end = pmm_base_phys + pmm_pages * PAGE;
    uint64_t lo = base_address & ~(PAGE - 1);
    uint64_t hi = (base_address + size + PAGE - 1) & ~(PAGE - 1);
    if (lo < pmm_base_phys) lo = pmm_base_phys;
    if (hi > pool_end) hi = pool_end;
    if (lo < hi) {
        uint64_t idx = (lo - pmm_base_phys) / PAGE;
        uint64_t n = (hi - lo) / PAGE;
        for (uint64_t i = idx; i < idx + n; i++) {
            if (!bit_test(i)) { bit_set(i); pmm_used++; }
        }
    }
    irq_restore(f);
}

uint64_t pmm_alloc_pages_aligned(uint64_t count, uint64_t alignment) {
    uint64_t f = irq_save();
    uint64_t r = alloc_run(count, alignment);
    irq_restore(f);
    return r;
}

uint64_t pmm_alloc_pages(uint64_t count) { return pmm_alloc_pages_aligned(count, PAGE); }

void pmm_free_pages(uint64_t phys_addr, uint64_t count) {
    uint64_t f = irq_save();
    free_run(phys_addr, count);
    irq_restore(f);
}

uint64_t pmm_alloc_page(void) {
    uint64_t f = irq_save();
    uint64_t r = alloc_one();
    irq_restore(f);
    return r;
}

uint64_t pmm_alloc_page_zero(void) {
    uint64_t p = pmm_alloc_page();
    /* Identity map: physical == virtual for the managed pool. */
    if (p) memset((void*)p, 0, (u32)PAGE);
    return p;
}

void pmm_free_page(uint64_t phys_addr) { pmm_free_pages(phys_addr, 1); }

uint64_t pmm_alloc(uint64_t size, uint64_t alignment) {
    if (size == 0) return 0;
    uint64_t pages = (size + PAGE - 1) / PAGE;
    if (alignment < PAGE) alignment = PAGE;
    return pmm_alloc_pages_aligned(pages, alignment);
}

void pmm_free(uint64_t phys_addr, uint64_t size) {
    pmm_free_pages(phys_addr, (size + PAGE - 1) / PAGE);
}

uint64_t pmm_total_pages(void)      { return pmm_pages; }
uint64_t pmm_used_pages(void)       { return pmm_used; }
uint64_t pmm_free_pages_count(void) { return pmm_pages - pmm_used; }
