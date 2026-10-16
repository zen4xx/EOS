#include "pci.h"
#include "ports.h"  

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_address(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (uint32_t)(
        (1u << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)(device & 0x1F) << 11) |
        ((uint32_t)(func & 0x07) << 8) |
        ((uint32_t)(offset & 0xFC))
    );
}

uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    port_long_out(PCI_CONFIG_ADDRESS, pci_address(bus, device, func, offset));
    return port_long_in(PCI_CONFIG_DATA);
}

void pci_write32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    port_long_out(PCI_CONFIG_ADDRESS, pci_address(bus, device, func, offset));
    port_long_out(PCI_CONFIG_DATA, value);
}

int pci_find_ahci(pci_device_t *out) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint8_t max_func = 1;
            
            uint32_t vendor_device = pci_read32(bus, dev, 0, 0x00);
            if ((vendor_device & 0xFFFF) == 0xFFFF) continue;
            
            uint8_t header_type = (pci_read32(bus, dev, 0, 0x0C) >> 16) & 0xFF;
            if (header_type & 0x80) max_func = 8;
            
            for (uint8_t func = 0; func < max_func; func++) {
                vendor_device = pci_read32(bus, dev, func, 0x00);
                if ((vendor_device & 0xFFFF) == 0xFFFF) continue;
                
                uint32_t class_reg = pci_read32(bus, dev, func, 0x08);
                uint8_t class_code = (class_reg >> 24) & 0xFF;
                uint8_t subclass = (class_reg >> 16) & 0xFF;
                uint8_t prog_if = (class_reg >> 8) & 0xFF;
                
                if (class_code == 0x01 && subclass == 0x06 && prog_if == 0x01) {
                    out->bus = bus;
                    out->device = dev;
                    out->func = func;
                    out->vendor_id = vendor_device & 0xFFFF;
                    out->device_id = (vendor_device >> 16) & 0xFFFF;
                    out->class_code = class_code;
                    out->subclass = subclass;
                    out->prog_if = prog_if;
                    
                    uint32_t bar5_low = pci_read32(bus, dev, func, 0x24);
                    uint32_t bar5_high = 0;
                    
                    if ((bar5_low & 0x07) == 0x04) {
                        bar5_high = pci_read32(bus, dev, func, 0x28);
                    }
                    
                    out->bar5 = ((uint64_t)bar5_high << 32) | (bar5_low & 0xFFFFFFF0);
                    
                    uint16_t cmd = pci_read32(bus, dev, func, 0x04) & 0xFFFF;
                    cmd |= 0x06;
                    pci_write32(bus, dev, func, 0x04, cmd);
                    
                    return 0;
                }
            }
        }
    }
    return -1;
}