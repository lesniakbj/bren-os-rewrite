#ifndef ACPI_H
#define ACPI_H

#include <libc/stdint.h>
#include <libc/strings.h>

// Root System Description Pointer (RSDP)
typedef struct {
    char signature[8];
    kuint8_t checksum;
    char oem_id[6];
    kuint8_t revision;
    kuint32_t rsdt_address;
} __attribute__((packed)) rsdp_descriptor_t;

// Extended Root System Description Pointer (RSDP for ACPI 2.0+)
typedef struct {
    rsdp_descriptor_t first_part;
    kuint32_t length;
    kuint64_t xsdt_address;
    kuint8_t extended_checksum;
    kuint8_t reserved[3];
} __attribute__((packed)) rsdp_descriptor_20_t;

// Generic ACPI Table Header
typedef struct {
    char signature[4];
    kuint32_t length;
    kuint8_t revision;
    kuint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    kuint32_t oem_revision;
    kuint32_t creator_id;
    kuint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

// Root System Description Table (RSDT)
typedef struct {
    acpi_sdt_header_t header;
    kuint32_t pointers_to_other_sdt[];
} __attribute__((packed)) rsdt_t;

// Extended System Description Table (XSDT)
typedef struct {
    acpi_sdt_header_t header;
    kuint64_t pointers_to_other_sdt[];
} __attribute__((packed)) xsdt_t;


void acpi_init();
void* acpi_find_table(char* signature);

#endif //ACPI_H
