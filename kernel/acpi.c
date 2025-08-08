#include <kernel/acpi.h>
#include <arch/i386/memory.h>
#include <drivers/terminal.h>
#include <libc/strings.h>

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
    terminal_writestring("ACPI initialization...\n");

    // 1. Find RSDP
    kuint16_t ebda_segment = *(kuint16_t*)0x40E;
    physical_addr_t ebda_addr = (physical_addr_t)ebda_segment << 4;
    rsdp = (rsdp_descriptor_20_t*) memsearch_aligned("RSD PTR ", ebda_addr, ebda_addr + 1024 - 1, 16);

    if (rsdp == 0) {
        rsdp = (rsdp_descriptor_20_t*) memsearch_aligned("RSD PTR ", 0xE0000, 0xFFFFF, 16);
    }

    if (rsdp == 0) {
        terminal_writestring("ACPI RSDP not found.\n");
        return;
    }

    terminal_writestringf("ACPI RSDP found at 0x%x, revision %d\n", (kuint32_t)rsdp, rsdp->first_part.revision);

    // 2. Check ACPI Version and get the root table
    if (rsdp->first_part.revision >= 2) { // ACPI 2.0+
        xsdt = (xsdt_t*)((physical_addr_t)rsdp->xsdt_address);
        terminal_writestringf("XSDT should be at address 0x%x\n", (kuint32_t)xsdt);
        if (!acpi_validate_checksum(&xsdt->header)) {
            terminal_writestring("XSDT checksum is invalid. Aborting.\n");
            xsdt = 0;
        }
    } else { // ACPI 1.0
        rsdt = (rsdt_t*)((physical_addr_t)rsdp->first_part.rsdt_address);
        terminal_writestringf("RSDT should be at address 0x%x\n", (kuint32_t)rsdt);
        if (!acpi_validate_checksum(&rsdt->header)) {
            terminal_writestring("RSDT checksum is invalid. Aborting.\n");
            rsdt = 0;
        }
    }
}

void* acpi_find_table(char* signature) {
    terminal_writestringf("Searching for ACPI table with signature: %s\n", signature);
    if (xsdt != 0) { // Use XSDT if available (ACPI 2.0+)
        int entries = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / 8;
        terminal_writestringf("XSDT has %d entries.\n", entries);
        for (int i = 0; i < entries; i++) {
            acpi_sdt_header_t* table = (acpi_sdt_header_t*)((physical_addr_t)xsdt->pointers_to_other_sdt[i]);
            terminal_writestringf("  Entry %d: Signature '%.4s', Address 0x%x\n", i, table->signature, (kuint32_t)table);
            if (strncmp(table->signature, signature, 4) == 0) {
                if (acpi_validate_checksum(table)) {
                    terminal_writestringf("Found valid table %s at 0x%x\n", signature, (kuint32_t)table);
                    return table;
                } else {
                    terminal_writestringf("Found table %s, but checksum is invalid.\n", signature);
                }
            }
        }
    } else if (rsdt != 0) { // Fallback to RSDT for ACPI 1.0
        int entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
        terminal_writestringf("RSDT has %d entries.\n", entries);
        for (int i = 0; i < entries; i++) {
            acpi_sdt_header_t* table = (acpi_sdt_header_t*)((physical_addr_t)rsdt->pointers_to_other_sdt[i]);
            terminal_writestringf("  Entry %d: Signature '%.4s', Address 0x%x\n", i, table->signature, (kuint32_t)table);
            if (strncmp(table->signature, signature, 4) == 0) {
                if (acpi_validate_checksum(table)) {
                    terminal_writestringf("Found valid table %s at 0x%x\n", signature, (kuint32_t)table);
                    return table;
                } else {
                    terminal_writestringf("Found table %s, but checksum is invalid.\n", signature);
                }
            }
        }
    }

    terminal_writestringf("Table %s not found.\n", signature);
    return 0;
 }
