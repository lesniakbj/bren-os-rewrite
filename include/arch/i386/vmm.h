#ifndef ARCH_I386_VMM_H
#define ARCH_I386_VMM_H

#include <libc/stdint.h>
#include <libc/strings.h>
#include <kernel/multiboot.h>
#include <arch/i386/interrupts.h>

#define PAGE_SIZE 4096
#define TABLE_ENTRIES 1024

// Page Directory/Table Entry Flags
#define PTE_PRESENT     0x01
#define PTE_READ_WRITE  0x02
#define PTE_USER        0x04        // User/Supervisor
#define PTE_WRITE_THROUGH 0x08
#define PTE_CACHE_DISABLE 0x10
#define PTE_ACCESSED    0x20
#define PTE_DIRTY       0x40        // Page Table Entry only
#define PTE_FRAME       0xFFFFF000  // Frame address mask

// Page Directory Entry Flags
#define PDE_PRESENT     0x01
#define PDE_READ_WRITE  0x02
#define PDE_USER        0x04
#define PDE_WRITE_THROUGH 0x08
#define PDE_CACHE_DISABLE 0x10
#define PDE_ACCESSED    0x20
#define PDE_PAGE_SIZE   0x80        // 0 for 4KB, 1 for 4MB
#define PDE_FRAME       0xFFFFF000

typedef kuint32_t pte_t;
typedef kuint32_t pde_t;

typedef struct vmm_init_status {
    bool is_init;
} vmm_init_status_t;


vmm_init_status_t vmm_init(multiboot_info_t* mbi);
void vmm_identity_map_page(physical_addr_t physical_addr);
void vmm_map_page(virtual_addr_t virtual_addr, physical_addr_t physical_addr, kuint32_t flags);
void vmm_unmap_page(virtual_addr_t virtual_addr);
physical_addr_t vmm_get_physical_addr(virtual_addr_t virtual_addr);
pde_t* vmm_get_kernel_directory();
void page_fault_handler(registers_t *regs);

#endif //ARCH_I386_VMM_H
