#include <kernel/acpi.h>
#include <kernel/log.h>
#include <arch/i386/pmm.h>
#include <arch/i386/vmm.h>
#include <arch/i386/memory.h>

static rsdp_descriptor_20_t* rsdp = 0;
static rsdt_t* rsdt = 0;
static xsdt_t* xsdt = 0;

static bool acpi_validate_checksum(acpi_sdt_header_t* table_header) {
    kuint8_t sum = 0;
    for(kuint32_t i = 0; i < table_header->length; i++) {
        sum += ((char*)table_header)[i];
    }
    return sum == 0;
}

void acpi_init() {
    LOG_INFO("ACPI initialization...\n");

    // 1. Find RSDP
    volatile kuint16_t* ebda_ptr = (volatile kuint16_t*)0x40E;
    kuint16_t ebda_segment = *ebda_ptr;
    physical_addr_t ebda_addr = (physical_addr_t)ebda_segment << 4;
    rsdp = (rsdp_descriptor_20_t*) memsearch_aligned("RSD PTR ", ebda_addr, ebda_addr + 1024 - 1, 16);

    if (rsdp == 0) {
        rsdp = (rsdp_descriptor_20_t*) memsearch_aligned("RSD PTR ", 0xE0000, 0xFFFFF, 16);
    }

    if (rsdp == 0) {
        LOG_ERR("ACPI RSDP not found.\n");
        return;
    }

    LOG_INFO("ACPI RSDP found at 0x%x, revision %d\n", (kuint32_t)rsdp, rsdp->first_part.revision);

    // 2. Check ACPI Version and get the root table
    if (rsdp->first_part.revision >= 2) { // ACPI 2.0+
        xsdt = (xsdt_t*)((physical_addr_t)rsdp->xsdt_address);
        LOG_INFO("XSDT should be at address 0x%x\n", (kuint32_t)xsdt);
        vmm_identity_map_page((physical_addr_t)xsdt);
        if (!acpi_validate_checksum(&xsdt->header)) {
            LOG_ERR("XSDT checksum is invalid. Aborting.\n");
            xsdt = 0;
        }
    } else { // ACPI 1.0
        rsdt = (rsdt_t*)((physical_addr_t)rsdp->first_part.rsdt_address);
        LOG_INFO("RSDT should be at address 0x%x\n", (kuint32_t)rsdt);
        vmm_identity_map_page((physical_addr_t)rsdt);
        if (!acpi_validate_checksum(&rsdt->header)) {
            LOG_ERR("RSDT checksum is invalid. Aborting.\n");
            rsdt = 0;
        }
    }
}

void* acpi_find_table(char* signature) {
    LOG_INFO("Searching for ACPI table with signature: %s\n", signature);
    if (xsdt != 0) { // Use XSDT if available (ACPI 2.0+)
        int entries = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / 8;
        LOG_INFO("XSDT has %d entries.\n", entries);
        for (int i = 0; i < entries; i++) {
            acpi_sdt_header_t* table = (acpi_sdt_header_t*)((physical_addr_t)xsdt->pointers_to_other_sdt[i]);
            vmm_identity_map_page((physical_addr_t)table);
            LOG_INFO("\tEntry %d: Signature '%.4s', Address 0x%x\n", i, table->signature, (kuint32_t)table);
            if (strncmp(table->signature, signature, 4) == 0) {
                if (acpi_validate_checksum(table)) {
                    LOG_INFO("Found valid table %s at 0x%x\n", signature, (kuint32_t)table);
                    return table;
                } else {
                    LOG_ERR("Found table %s, but checksum is invalid.\n", signature);
                }
            }
        }
    } else if (rsdt != 0) { // Fallback to RSDT for ACPI 1.0
        int entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
        LOG_INFO("RSDT has %d entries.\n", entries);
        for (int i = 0; i < entries; i++) {
            acpi_sdt_header_t* table = (acpi_sdt_header_t*)((physical_addr_t)rsdt->pointers_to_other_sdt[i]);
            vmm_identity_map_page((physical_addr_t)table);
            LOG_INFO("\tEntry %d: Signature '%.4s', Address 0x%x\n", i, table->signature, (kuint32_t)table);
            if (strncmp(table->signature, signature, 4) == 0) {
                if (acpi_validate_checksum(table)) {
                    LOG_INFO("Found valid table %s at 0x%x\n", signature, (kuint32_t)table);
                    return table;
                } else {
                    LOG_ERR("Found table %s, but checksum is invalid.\n", signature);
                }
            }
        }
    }

    LOG_INFO("Table %s not found.\n", signature);
 }
