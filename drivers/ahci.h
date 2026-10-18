#ifndef AHCI_H
#define AHCI_H

#include "../libc/stdint.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} volatile HBA_PORT;

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t  rsv[0xA0 - 0x2C];
    uint8_t  vendor[0x100 - 0xA0];
    HBA_PORT ports[32];
} volatile HBA_MEM;

typedef struct {
    uint32_t cfl:5;
    uint32_t a:1;
    uint32_t w:1;
    uint32_t p:1;
    uint32_t r:1;
    uint32_t b:1;
    uint32_t c:1;
    uint32_t rsv0:1;
    uint32_t pmp:4;
    uint32_t prdtl:16;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} volatile HBA_CMD_HEADER;

typedef struct {
    uint8_t  fis_type;
    uint8_t  pmport:4;
    uint8_t  rsv0:3;
    uint8_t  c:1;
    uint8_t  command;
    uint8_t  featurel;
    uint8_t  lba0;
    uint8_t  lba1;
    uint8_t  lba2;
    uint8_t  device;
    uint8_t  lba3;
    uint8_t  lba4;
    uint8_t  lba5;
    uint8_t  featureh;
    uint8_t  countl;
    uint8_t  counth;
    uint8_t  icc;
    uint8_t  control;
    uint8_t  rsv1[4];
} volatile HBA_FIS_REG_H2D;

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc:22;
    uint32_t rsv1:9;
    uint32_t i:1;
} volatile HBA_PRDT_ENTRY;

typedef struct {
    uint8_t  cfis[64];
    uint8_t  acmd[16];
    uint8_t  rsv[48];
    HBA_PRDT_ENTRY prdt_entry[1];
} volatile HBA_CMD_TBL;

#pragma pack(pop)

#define ATA_CMD_READ_DMA_EX     0x25
#define ATA_CMD_WRITE_DMA_EX    0x35
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA

#define AHCI_TIMEOUT 1000000

int ahci_init(void);
int ahci_read_sector(uint64_t lba, uint8_t *buffer, uint32_t count);
int ahci_write_sector(uint64_t lba, const uint8_t *buffer, uint32_t count);
int ahci_flush_cache(void);

#endif