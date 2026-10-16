#ifndef PCI_H
#define PCI_H

#include "../libc/stdint.h"

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint64_t bar5;
} pci_device_t;

uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void pci_write32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);
int pci_find_ahci(pci_device_t *out);

#endif