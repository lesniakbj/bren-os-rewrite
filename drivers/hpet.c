#include <drivers/hpet.h>
#include <kernel/log.h>
#include <arch/i386/vmm.h>

static kuint64_t hpet_read64(volatile void* hpet_base, kuint64_t reg) {
    return *((volatile kuint64_t*)((char*)hpet_base + reg));
}

static void hpet_write64(volatile void* hpet_base, kuint64_t reg, kuint64_t value) {
    *((volatile kuint64_t*)((char*)hpet_base + reg)) = value;
}

int hpet_init() {
    LOG_INFO("HPET driver initialization...\n");

    hpet_acpi_table_t* hpet_table = (hpet_acpi_table_t*) acpi_find_table("HPET");

    if (hpet_table != 0) {
        LOG_INFO("HPET ACPI table found.\n");

        if (hpet_table->base_address.address_space_id != 0) {
            LOG_ERR("HPET is not memory-mapped. Aborting.\n");
            return -1;
        }

        physical_addr_t hpet_base_address = (physical_addr_t)hpet_table->base_address.address;
        LOG_INFO("HPET base address: 0x%x\n", hpet_base_address);

        // Map the HPET's physical memory into the virtual address space.
        vmm_identity_map_page(hpet_base_address);

        volatile void* hpet_addr = (volatile void*)hpet_base_address;

        kuint64_t caps = hpet_read64(hpet_addr, HPET_GENERAL_CAPS_ID);
        kuint32_t period_femtoseconds = (kuint32_t)((kuint64_t)caps >> 32);
        LOG_INFO("HPET counter period (femtoseconds): %d\n", period_femtoseconds);

        if (period_femtoseconds == 0 || period_femtoseconds > 100000000) { // Sanity check the period
            LOG_ERR("Error: HPET period is invalid. Hardware may not be enabled by BIOS.\n");
            return -1;
        }

        // The following line will likely crash your printf due to 64-bit integer division/printing.
        // It is commented out for safety. To use it, you need a 64-bit itoa and printf implementation.
        // kuint64_t hpet_frequency_hz = 1000000000000000 / period_femtoseconds;
        // terminal_write_stringf("HPET frequency: %d Hz\n", hpet_frequency_hz);

    } else {
        LOG_INFO("HPET ACPI table not found. Falling back to PIT.\n");
    }

    return 0;
}

