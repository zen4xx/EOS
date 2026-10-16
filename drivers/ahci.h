#ifndef AHCI_H
#define AHCI_H

#include "../libc/stdint.h"

#pragma pack(push, 1)

typedef struct {
    uint32_t clb;        // 0x00: Command list base address (Physical!)
    uint32_t clbu;       // 0x04: Command list base address upper 32 bits
    uint32_t fb;         // 0x08: FIS base address (Physical!)
    uint32_t fbu;        // 0x0C: FIS base address upper 32 bits
    uint32_t is;         // 0x10: Interrupt status
    uint32_t ie;         // 0x14: Interrupt enable
    uint32_t cmd;        // 0x18: Command and status
    uint32_t rsv0;       // 0x1C: Reserved
    uint32_t tfd;        // 0x20: Task file data
    uint32_t sig;        // 0x24: Signature
    uint32_t ssts;       // 0x28: SATA status (SCR0: SStatus)
    uint32_t sctl;       // 0x2C: SATA control (SCR2: SControl)
    uint32_t serr;       // 0x30: SATA error (SCR1: SError)
    uint32_t sact;       // 0x34: SATA active (SCR3: SActive)
    uint32_t ci;         // 0x38: Command issue
    uint32_t sntf;       // 0x3C: SATA notification (SCR4: SNotification)
    uint32_t fbs;        // 0x40: FIS-based switch control
    uint32_t rsv1[11];   // 0x44 - 0x6F: Reserved
    uint32_t vendor[4];  // 0x70 - 0x7F: Vendor specific
} volatile HBA_PORT;

typedef struct {
    uint32_t cap;        // 0x00: Host capabilities
    uint32_t ghc;        // 0x04: Global host control
    uint32_t is;         // 0x08: Interrupt status
    uint32_t pi;         // 0x0C: Port implemented
    uint32_t vs;         // 0x10: Version
    uint32_t ccc_ctl;    // 0x14: Command completion coalescing control
    uint32_t ccc_pts;    // 0x18: Command completion coalescing ports
    uint32_t em_loc;     // 0x1C: Enclosure management location
    uint32_t em_ctl;     // 0x20: Enclosure management control
    uint32_t cap2;       // 0x24: Host capabilities extended
    uint32_t bohc;       // 0x28: BIOS/OS handoff control and status
    uint8_t  rsv[0xA0 - 0x2C];
    uint8_t  vendor[0x100 - 0xA0];
    HBA_PORT ports[32]; // 0x100: Port control registers
} volatile HBA_MEM;

typedef struct {
    uint32_t cfl:5;      // Command FIS length in DWORDS
    uint32_t a:1;        // ATAPI
    uint32_t w:1;        // Write
    uint32_t p:1;        // Prefetchable
    uint32_t r:1;        // Reset
    uint32_t b:1;        // BIST
    uint32_t c:1;        // Clear busy upon R_OK
    uint32_t rsv0:1;     // Reserved
    uint32_t pmp:4;      // Port multiplier port
    uint32_t prdtl:16;   // Physical region descriptor table length
    uint32_t prdbc;      // Physical region descriptor byte count
    uint32_t ctba;       // Command table descriptor base address (Physical!)
    uint32_t ctbau;      // Command table descriptor base address upper 32 bits
    uint32_t rsv1[4];    // Reserved
} volatile HBA_CMD_HEADER;

typedef struct {
    uint8_t  fis_type;   // 0x27 для Register H2D
    uint8_t  pmport:4;   // Port multiplier
    uint8_t  rsv0:3;     // Reserved
    uint8_t  c:1;        // 1 = Command, 0 = Control
    uint8_t  command;    // command ATA
    uint8_t  featurel;   // Feature (low)
    uint8_t  lba0;       // LBA [7:0]
    uint8_t  lba1;       // LBA [15:8]
    uint8_t  lba2;       // LBA [23:16]
    uint8_t  device;     // Device
    uint8_t  lba3;       // LBA [31:24]
    uint8_t  lba4;       // LBA [39:32]
    uint8_t  lba5;       // LBA [47:40]
    uint8_t  featureh;   // Feature (high)
    uint8_t  countl;     // Count (low)
    uint8_t  counth;     // Count (high)
    uint8_t  icc;        // Isochronous command completion
    uint8_t  control;    // Control
    uint8_t  rsv1[4];    // Reserved
} volatile HBA_FIS_REG_H2D;

typedef struct {
    uint32_t dba;        // Data base address (Physical!)
    uint32_t dbau;       // Data base address upper 32 bits
    uint32_t rsv0;       // Reserved
    uint32_t dbc:22;     // Byte count
    uint32_t rsv1:9;     // Reserved
    uint32_t i:1;        // Interrupt on completion
} volatile HBA_PRDT_ENTRY;

typedef struct {
    uint8_t  cfis[64];   // Command FIS
    uint8_t  acmd[16];   // ATAPI command
    uint8_t  rsv[48];    // Reserved
    HBA_PRDT_ENTRY prdt_entry[1];
} volatile HBA_CMD_TBL;

#pragma pack(pop)

#define ATA_CMD_READ_DMA_EX  0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA

int ahci_init(void);
int start_port(HBA_PORT *port);
int ahci_read_sector(uint64_t lba, uint8_t *buffer, uint32_t count);
int ahci_write_sector(uint64_t lba, const uint8_t *buffer, uint32_t count);
int ahci_flush_cache();

#endif