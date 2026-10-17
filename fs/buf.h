#ifndef FS_BUF_H
#define FS_BUF_H
#include "../libc/stdint.h"

typedef struct {
    uint8_t  *data;
    uint32_t  block;
    uint32_t  bs;
    uint32_t  refs;
    uint32_t  age;
    uint8_t   dirty;
    uint8_t   valid;
} buf_t;

buf_t *bread (uint32_t block, uint32_t bs);
buf_t *bnew  (uint32_t block, uint32_t bs);
void   brelse(buf_t *b);
void   bdirty(buf_t *b);
int    bsync (void);
void   binvalidate(void);

#endif