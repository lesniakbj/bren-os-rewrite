#include <kernel/log.h>
#include <arch/i386/fault.h>

void general_protection_fault_handler(registers_t* regs) {
    LOG_ERR("GENERAL PROTECTION FAULT: System Halted.");
    for(;;);
}