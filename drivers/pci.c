#include <drivers/pci.h>
#include <drivers/terminal.h>
#include <arch/i386/io.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static kuint32_t pci_read_dword(kuint8_t bus, kuint8_t device, kuint8_t function, kuint8_t offset) {
    kuint32_t address = (kuint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_write_dword(kuint8_t bus, kuint8_t device, kuint8_t function, kuint8_t offset, kuint32_t value) {
    kuint32_t address = (kuint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static kuint16_t pci_get_vendor_id(kuint8_t bus, kuint8_t device, kuint8_t function) {
    kuint32_t reg = pci_read_dword(bus, device, function, 0x00);
    return reg & 0xFFFF;
}

static kuint16_t pci_get_device_id(kuint8_t bus, kuint8_t device, kuint8_t function) {
    kuint32_t reg = pci_read_dword(bus, device, function, 0x00);
    return reg >> 16;
}

static kuint8_t pci_get_class_code(kuint8_t bus, kuint8_t device, kuint8_t function) {
    kuint32_t reg = pci_read_dword(bus, device, function, 0x08);
    return (reg >> 24) & 0xFF;
}


static kuint8_t pci_get_subclass(kuint8_t bus, kuint8_t device, kuint8_t function) {
    kuint32_t reg = pci_read_dword(bus, device, function, 0x08);
    return (reg >> 16) & 0xFF;
}

const char* pci_class_subclass_to_string(kuint8_t class_code, kuint8_t subclass) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01:
            switch (subclass) {
                case 0x00: return "SCSI Bus Controller";
                case 0x01: return "IDE Controller";
                case 0x02: return "Floppy Disk Controller";
                case 0x03: return "IPI Bus Controller";
                case 0x04: return "RAID Controller";
                case 0x05: return "ATA Controller";
                case 0x06: return "Serial ATA";
                case 0x07: return "Serial Attached SCSI";
                case 0x08: return "Non-Volatile Memory Controller";
                default: return "Mass Storage Controller";
            }
        case 0x02:
            switch (subclass) {
                case 0x00: return "Ethernet Controller";
                case 0x01: return "Token Ring Controller";
                case 0x02: return "FDDI Controller";
                case 0x03: return "ATM Controller";
                case 0x04: return "ISDN Controller";
                case 0x05: return "WorldFip Controller";
                case 0x06: return "PICMG 2.14 Multi-Computing";
                case 0x07: return "Infiniband Controller";
                case 0x08: return "Fabric Controller";
                default: return "Network Controller";
            }
        case 0x03:
            switch (subclass) {
                case 0x00: return "VGA Compatible Controller";
                case 0x01: return "XGA Controller";
                case 0x02: return "3D Controller (Not VGA-Compatible)";
                default: return "Display Controller";
            }
        case 0x04:
            switch (subclass) {
                case 0x00: return "Multimedia Video Controller";
                case 0x01: return "Multimedia Audio Controller";
                case 0x02: return "Computer Telephony Device";
                case 0x03: return "Audio Device";
                default: return "Multimedia Controller";
            }
        case 0x05:
            switch (subclass) {
                case 0x00: return "RAM Controller";
                case 0x01: return "Flash Controller";
                default: return "Memory Controller";
            }
        case 0x06:
            switch (subclass) {
                case 0x00: return "Host Bridge";
                case 0x01: return "ISA Bridge";
                case 0x02: return "EISA Bridge";
                case 0x03: return "MCA Bridge";
                case 0x04: return "PCI-to-PCI Bridge";
                case 0x05: return "PCMCIA Bridge";
                case 0x06: return "NuBus Bridge";
                case 0x07: return "CardBus Bridge";
                case 0x08: return "RACEway Bridge";
                case 0x09: return "PCI-to-PCI Bridge";
                case 0x0A: return "InfiniBand-to-PCI Host Bridge";
                default: return "Bridge Device";
            }
        case 0x07:
            switch (subclass) {
                case 0x00: return "Serial Controller";
                case 0x01: return "Parallel Controller";
                case 0x02: return "Multiport Serial Controller";
                case 0x03: return "Modem";
                case 0x04: return "GPIB (IEEE 488.1/2) Controller";
                case 0x05: return "Smart Card";
                default: return "Simple Communication Controller";
            }
        case 0x08:
            switch (subclass) {
                case 0x00: return "PIC";
                case 0x01: return "DMA Controller";
                case 0x02: return "Timer";
                case 0x03: return "RTC Controller";
                case 0x04: return "PCI Hot-Plug Controller";
                case 0x05: return "SD Host controller";
                case 0x06: return "IOMMU";
                default: return "Base System Peripheral";
            }
        case 0x09:
            switch (subclass) {
                case 0x00: return "Keyboard Controller";
                case 0x01: return "Digitizer Pen";
                case 0x02: return "Mouse Controller";
                case 0x03: return "Scanner Controller";
                case 0x04: return "Gameport Controller";
                default: return "Input Device Controller";
            }
        case 0x0A:
            switch (subclass) {
                case 0x00: return "Generic";
                default: return "Docking Station";
            }
        case 0x0B:
            switch (subclass) {
                case 0x00: return "386";
                case 0x01: return "486";
                case 0x02: return "Pentium";
                case 0x10: return "Alpha";
                case 0x20: return "PowerPC";
                case 0x30: return "MIPS";
                case 0x40: return "Co-Processor";
                default: return "Processor";
            }
        case 0x0C:
            switch (subclass) {
                case 0x00: return "FireWire (IEEE 1394) Controller";
                case 0x01: return "ACCESS Bus";
                case 0x02: return "SSA";
                case 0x03: return "USB Controller";
                case 0x04: return "Fibre Channel";
                case 0x05: return "SMBus";
                case 0x06: return "InfiniBand";
                case 0x07: return "IPMI Interface";
                case 0x08: return "SERCOS Interface (IEC 61491)";
                case 0x09: return "CANbus";
                default: return "Serial Bus Controller";
            }
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing Controller";
        case 0x12: return "Processing Accelerator";
        case 0x13: return "Non-Essential Instrumentation";
        case 0x40: return "Co-Processor";
        case 0xFF: return "Unassigned Class (Vendor specific)";
        default: return "Unknown";
    }
}

void pci_init() {
    terminal_writestring("PCI bus scan...\n");

    for (kuint16_t bus = 0; bus < 256; bus++) {
        for (kuint8_t device = 0; device < 32; device++) {
            kuint16_t vendor_id = pci_get_vendor_id(bus, device, 0);
            if (vendor_id == 0xFFFF) { // Device doesn't exist
                continue;
            }
            kuint16_t device_id = pci_get_device_id(bus, device, 0);
            kuint8_t class_code = pci_get_class_code(bus, device, 0);
            kuint8_t subclass = pci_get_subclass(bus, device, 0);
            const char* class_str = pci_class_subclass_to_string(class_code, subclass);
            terminal_writestringf("Found PCI device %d:%d - V:0x%x, D:0x%x, %s\n",
                bus, device, vendor_id, device_id, class_str);

            // Check if this is the LPC bridge
            if (class_code == 0x06 && subclass == 0x01) {
                terminal_writestringf("LPC bridge found (Vendor: 0x%x, Device: 0x%x).\n", vendor_id, device_id);
                terminal_writestring("Attempting to enable HPET address decoder...\n");
                
                #define INTEL_LPC_HPTC_REGISTER 0x104
                kuint32_t hpet_conf_before = pci_read_dword(bus, device, 0, INTEL_LPC_HPTC_REGISTER);
                
                kuint32_t hpet_conf_to_write = hpet_conf_before | (1 << 7); // Prepare to set bit 7
                pci_write_dword(bus, device, 0, INTEL_LPC_HPTC_REGISTER, hpet_conf_to_write);
                
                kuint32_t hpet_conf_readback = pci_read_dword(bus, device, 0, INTEL_LPC_HPTC_REGISTER);

                terminal_writestringf("  HPET Config Before: 0x%x\n", hpet_conf_before);
                terminal_writestringf("  HPET Config After Write: 0x%x\n", hpet_conf_readback);

                if (hpet_conf_readback & (1 << 7)) {
                    terminal_writestring("  HPET enable bit is set.\n");
                } else {
                    terminal_writestring("  Warning: HPET enable bit did not stick. Register may be read-only.\n");
                }
            }
        }
    }
    terminal_writestring("PCI bus scan complete.\n");
}

