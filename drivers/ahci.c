#include "ahci.h"
#include "../libc/memory.h"
#include "../kernel/pmm.h"
#include "pci.h"

uint64_t virt_to_phys(void *addr)
{
    return (uint64_t)addr;
}

#define MAKE_PHYS_ADDR(low, high) (((uint64_t)(high) << 32) | (uint32_t)(low))

#define SET_PHYS_ADDR(low, high, addr) do { \
    (low) = (uint32_t)((uint64_t)(addr) & 0xFFFFFFFF); \
    (high) = (uint32_t)(((uint64_t)(addr) >> 32) & 0xFFFFFFFF); \
} while(0)

static HBA_MEM *hba_mem = 0;
static HBA_PORT *ahci_port = 0;

static int wait_port_ready(HBA_PORT *port)
{
    int timeout = AHCI_TIMEOUT;
    
    while ((port->tfd & ((1 << 7) | (1 << 3))) && timeout > 0) {
        timeout--;
    }
    
    return (timeout > 0) ? 0 : -1;
}

static int wait_command_complete(HBA_PORT *port)
{
    int timeout = AHCI_TIMEOUT;
    
    while ((port->ci & 1) && timeout > 0) {
        if (port->is & (1 << 30)) {
            return -1;
        }
        timeout--;
    }
    
    if (timeout == 0) {
        return -1;
    }
    
    if (port->is & (1 << 0)) {
        port->is = (1 << 0);
        return 0;
    }
    
    return -1;
}

static int start_port(HBA_PORT *port)
{
    port->cmd &= ~(1 << 4);
    port->cmd &= ~(1 << 0);

    int timeout = AHCI_TIMEOUT;
    while ((port->cmd & ((1 << 14) | (1 << 15))) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        return -1;
    }

    uint64_t clb_phys = pmm_alloc(1024, 1024);
    if (!clb_phys) {
        return -1;
    }
    
    port->clb = (uint32_t)(clb_phys & 0xFFFFFFFF);
    port->clbu = (uint32_t)((clb_phys >> 32) & 0xFFFFFFFF);

    uint64_t fb_phys = pmm_alloc(256, 256);
    if (!fb_phys) {
        return -1;
    }
    
    port->fb = (uint32_t)(fb_phys & 0xFFFFFFFF);
    port->fbu = (uint32_t)((fb_phys >> 32) & 0xFFFFFFFF);

    memset((void*)clb_phys, 0, 1024);
    memset((void*)fb_phys, 0, 256);

    HBA_CMD_HEADER *cmd_headers = (HBA_CMD_HEADER*)clb_phys;
    
    for (int i = 0; i < 32; i++) {
        uint64_t ctba_phys = pmm_alloc(256, 256);
        if (!ctba_phys) {
            return -1;
        }
        
        cmd_headers[i].cfl = sizeof(HBA_FIS_REG_H2D) / sizeof(uint32_t);
        cmd_headers[i].w = 0;
        cmd_headers[i].prdtl = 0;
        cmd_headers[i].ctba = (uint32_t)(ctba_phys & 0xFFFFFFFF);
        cmd_headers[i].ctbau = (uint32_t)((ctba_phys >> 32) & 0xFFFFFFFF);
        
        memset((void*)ctba_phys, 0, 256);
    }

    port->cmd |= (1 << 4);
    port->cmd |= (1 << 0);

    timeout = AHCI_TIMEOUT;
    while (!(port->cmd & ((1 << 14) | (1 << 15))) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        return -1;
    }

    return 0;
}

int ahci_read_sector(uint64_t lba, uint8_t *buffer, uint32_t count)
{
    if (!ahci_port || !buffer || count == 0) {
        return -1;
    }
    
    if (count * 512 > 4 * 1024 * 1024) {
        return -1;
    }
    
    if (wait_port_ready(ahci_port) != 0) {
        return -1;
    }

    uint64_t buffer_phys = virt_to_phys(buffer);
    
    uint64_t clb_phys = MAKE_PHYS_ADDR(ahci_port->clb, ahci_port->clbu);
    HBA_CMD_HEADER *cmd_header = (HBA_CMD_HEADER*)clb_phys;
    
    uint64_t ctba_phys = MAKE_PHYS_ADDR(cmd_header->ctba, cmd_header->ctbau);
    HBA_CMD_TBL *cmd_tbl = (HBA_CMD_TBL*)ctba_phys;

    cmd_header->w = 0;
    cmd_header->prdtl = 1;

    SET_PHYS_ADDR(cmd_tbl->prdt_entry[0].dba, cmd_tbl->prdt_entry[0].dbau, buffer_phys);
    cmd_tbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmd_tbl->prdt_entry[0].i = 1;

    HBA_FIS_REG_H2D *cmd_fis = (HBA_FIS_REG_H2D*)(&cmd_tbl->cfis);
    cmd_fis->fis_type = 0x27;
    cmd_fis->c = 1;
    cmd_fis->command = ATA_CMD_READ_DMA_EX;

    cmd_fis->lba0 = (uint8_t)(lba & 0xFF);
    cmd_fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    cmd_fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    cmd_fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    cmd_fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    cmd_fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);
    
    cmd_fis->device = 1 << 6;
    cmd_fis->countl = count & 0xFF;
    cmd_fis->counth = (count >> 8) & 0xFF;

    ahci_port->is = (uint32_t)-1;
    ahci_port->ci = 1;

    return wait_command_complete(ahci_port);
}

int ahci_write_sector(uint64_t lba, const uint8_t *buffer, uint32_t count)
{
    if (!ahci_port || !buffer || count == 0) {
        return -1;
    }
    
    if (count * 512 > 4 * 1024 * 1024) {
        return -1;
    }
    
    if (wait_port_ready(ahci_port) != 0) {
        return -1;
    }

    uint64_t buffer_phys = virt_to_phys((void*)buffer);
    
    uint64_t clb_phys = MAKE_PHYS_ADDR(ahci_port->clb, ahci_port->clbu);
    HBA_CMD_HEADER *cmd_header = (HBA_CMD_HEADER*)clb_phys;
    
    uint64_t ctba_phys = MAKE_PHYS_ADDR(cmd_header->ctba, cmd_header->ctbau);
    HBA_CMD_TBL *cmd_tbl = (HBA_CMD_TBL*)ctba_phys;

    cmd_header->w = 1;
    cmd_header->prdtl = 1;

    SET_PHYS_ADDR(cmd_tbl->prdt_entry[0].dba, cmd_tbl->prdt_entry[0].dbau, buffer_phys);
    cmd_tbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmd_tbl->prdt_entry[0].i = 1;

    HBA_FIS_REG_H2D *cmd_fis = (HBA_FIS_REG_H2D*)(&cmd_tbl->cfis);
    cmd_fis->fis_type = 0x27;
    cmd_fis->c = 1;
    cmd_fis->command = ATA_CMD_WRITE_DMA_EX;

    cmd_fis->lba0 = (uint8_t)(lba & 0xFF);
    cmd_fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    cmd_fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    cmd_fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    cmd_fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    cmd_fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);
    
    cmd_fis->device = 1 << 6;
    cmd_fis->countl = count & 0xFF;
    cmd_fis->counth = (count >> 8) & 0xFF;

    ahci_port->is = (uint32_t)-1;
    ahci_port->ci = 1;

    return wait_command_complete(ahci_port);
}

int ahci_flush_cache(void)
{
    if (!ahci_port) {
        return -1;
    }
    
    if (wait_port_ready(ahci_port) != 0) {
        return -1;
    }

    uint64_t clb_phys = MAKE_PHYS_ADDR(ahci_port->clb, ahci_port->clbu);
    HBA_CMD_HEADER *cmd_header = (HBA_CMD_HEADER*)clb_phys;
    
    uint64_t ctba_phys = MAKE_PHYS_ADDR(cmd_header->ctba, cmd_header->ctbau);
    HBA_CMD_TBL *cmd_tbl = (HBA_CMD_TBL*)ctba_phys;

    cmd_header->w = 0;
    cmd_header->prdtl = 0;

    HBA_FIS_REG_H2D *cmd_fis = (HBA_FIS_REG_H2D*)(&cmd_tbl->cfis);
    cmd_fis->fis_type = 0x27;
    cmd_fis->c = 1;
    cmd_fis->command = ATA_CMD_FLUSH_CACHE_EXT;
    cmd_fis->device = 1 << 6;

    ahci_port->is = (uint32_t)-1;
    ahci_port->ci = 1;

    return wait_command_complete(ahci_port);
}

int ahci_init(void)
{
    pci_device_t dev;
    
    if (pci_find_ahci(&dev) != 0) {
        return -1;
    }
    
    hba_mem = (HBA_MEM*)dev.bar5;
    
    if (!hba_mem) {
        return -1;
    }
    
    uint32_t pi = hba_mem->pi;
    int port_num = -1;
    
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            HBA_PORT *port = &hba_mem->ports[i];
            uint32_t ssts = port->ssts;
            uint8_t ipm = (ssts >> 8) & 0x0F;
            uint8_t det = ssts & 0x0F;
            
            if (det == 3 && ipm == 1) {
                port_num = i;
                ahci_port = port;
                break;
            }
        }
    }
    
    if (port_num == -1) {
        return -2;
    }
    
    if (start_port(ahci_port) != 0) {
        return -3;
    }
    
    return 0;
}