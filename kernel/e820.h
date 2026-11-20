#ifndef E820_H
#define E820_H

#include "../libc/stdint.h"

#define E820_BUF_ADDR 0x8800ULL
#define E820_MAGIC    0x30323845u        /* "E820" */
#define E820_MAX      64

#define E820_USABLE       1
#define E820_RESERVED     2
#define E820_ACPI_RECLAIM 3
#define E820_ACPI_NVS     4
#define E820_BAD          5

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t count;
    e820_entry_t entries[E820_MAX];
} __attribute__((packed)) e820_map_t;

static inline const e820_map_t* e820_validate(const e820_map_t* m) {
    if (m->magic != E820_MAGIC || m->count == 0 || m->count > E820_MAX) return 0;
    return m;
}

static inline const e820_map_t* e820_get(void) {
    return e820_validate((const e820_map_t*)E820_BUF_ADDR);
}

static inline uint64_t e820_total_usable(const e820_map_t* m) {
    uint64_t s = 0;
    for (uint32_t i = 0; i < m->count; i++)
        if (m->entries[i].type == E820_USABLE) s += m->entries[i].length;
    return s;
}

#endif /* E820_H */
