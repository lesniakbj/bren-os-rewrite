#ifndef HPET_H
#define HPET_H

#include <libc/stdint.h>
#include <kernel/acpi.h>

// ACPI Generic Address Structure (GAS)
typedef struct {
    kuint8_t address_space_id;    // 0 = System Memory, 1 = System I/O
    kuint8_t register_bit_width;
    kuint8_t register_bit_offset;
    kuint8_t access_size;
    kuint64_t address;
} __attribute__((packed)) generic_address_structure_t;


// ACPI HPET Description Table structure
typedef struct {
    acpi_sdt_header_t header;
    kuint32_t event_timer_block_id;
    generic_address_structure_t base_address;
    kuint8_t hpet_number;
    kuint16_t minimum_tick;
    kuint8_t page_protection;
} __attribute__((packed)) hpet_acpi_table_t;


// HPET Register Offsets
#define HPET_GENERAL_CAPS_ID      0x00
#define HPET_GENERAL_CONFIG       0x10
#define HPET_MAIN_COUNTER_VALUE   0xF0
#define HPET_TIMER0_CONFIG        0x100
#define HPET_TIMER0_COMPARATOR    0x108

void hpet_init();

#endif //HPET_H
