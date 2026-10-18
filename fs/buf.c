#include "buf.h"
#include "../drivers/ahci.h"
#include "../libc/memory.h"

#define BUF_COUNT 256
#define BUF_SIZE  4096

static buf_t bufs[BUF_COUNT];

static uint8_t pool[BUF_COUNT][BUF_SIZE] __attribute__((aligned(4096)));

static uint32_t clock_hand;

static buf_t *acquire(uint32_t block, uint32_t bs, int from_disk)
{
    if (!bs || bs > BUF_SIZE || (bs & 511)) return 0;

    for (int i = 0; i < BUF_COUNT; i++) {
        buf_t *b = &bufs[i];
        if (b->valid && b->block == block && b->bs == bs) {
            b->refs++;
            b->age = ++clock_hand;
            return b;
        }
    }

    int slot = -1;
    for (int i = 0; i < BUF_COUNT; i++) {
        if (!bufs[i].valid) { slot = i; break; }
    }

    if (slot < 0) {
        uint32_t best = 0xFFFFFFFF;
        for (int i = 0; i < BUF_COUNT; i++) {
            buf_t *b = &bufs[i];
            if (!b->valid || b->refs) continue;
            if (b->age < best) { best = b->age; slot = i; }
        }
    }

    if (slot < 0) return 0;

    buf_t *b = &bufs[slot];

    if (b->valid && b->dirty) {
        if (ahci_write_sector((uint64_t)b->block * (b->bs / 512),
                              b->data, b->bs / 512) != 0) {
            return 0;
        }
    }

    b->data  = pool[slot];
    b->block = block;
    b->bs    = bs;
    b->dirty = 0;
    b->valid = 1;
    b->refs  = 1;
    b->age   = ++clock_hand;

    if (from_disk) {
        if (ahci_read_sector((uint64_t)block * (bs / 512),
                             b->data, bs / 512) != 0) {
            b->valid = 0;
            b->refs  = 0;
            return 0;
        }
    } else {
        memset(b->data, 0, bs);
        b->dirty = 1;
    }
    return b;
}

buf_t *bread(uint32_t block, uint32_t bs) { return acquire(block, bs, 1); }
buf_t *bnew (uint32_t block, uint32_t bs) { return acquire(block, bs, 0); }

void bdirty(buf_t *b) { if (b) b->dirty = 1; }

void brelse(buf_t *b)
{
    if (!b || !b->refs) return;
    b->refs--;
}

int bsync(void)
{
    int errors = 0;
    for (int i = 0; i < BUF_COUNT; i++) {
        buf_t *b = &bufs[i];
        if (!b->valid || !b->dirty || b->refs) continue;

        if (ahci_write_sector((uint64_t)b->block * (b->bs / 512),
                              b->data, b->bs / 512) != 0) {
            errors++;
            continue;
        }
        b->dirty = 0;
    }
    return errors;
}

void binvalidate(void)
{
    bsync();
    for (int i = 0; i < BUF_COUNT; i++) {
        bufs[i].valid = 0;
        bufs[i].refs  = 0;
        bufs[i].dirty = 0;
    }
}