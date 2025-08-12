#ifndef ARCH_I386_VMM_H
#define ARCH_I386_VMM_H

#include <libc/stdint.h>
#include <kernel/multiboot.h>
#include <arch/i386/interrupts.h>

// Page Directory/Table Entry Flags
#define PTE_PRESENT     0x01
#define PTE_READ_WRITE  0x02
#define PTE_USER        0x04 // User/Supervisor
#define PTE_WRITE_THROUGH 0x08
#define PTE_CACHE_DISABLE 0x10
#define PTE_ACCESSED    0x20
#define PTE_DIRTY       0x40 // Page Table Entry only
#define PTE_FRAME       0xFFFFF000 // Frame address mask

// Page Directory Entry Flags
#define PDE_PRESENT     0x01
#define PDE_READ_WRITE  0x02
#define PDE_USER        0x04
#define PDE_WRITE_THROUGH 0x08
#define PDE_CACHE_DISABLE 0x10
#define PDE_ACCESSED    0x20
#define PDE_PAGE_SIZE   0x80 // 0 for 4KB, 1 for 4MB
#define PDE_FRAME       0xFFFFF000

typedef kuint32_t pte_t;
typedef kuint32_t pde_t;

/**
 * @brief Initializes the Virtual Memory Manager (VMM).
 * Sets up the initial page directory and page tables for the kernel,
 * identity mapping the first 4MB of memory. Then enables paging.
 * @param mbi The multiboot information structure, used to map framebuffer memory
 */
void vmm_init(multiboot_info_t* mbi);

/**
 * @brief Maps a 4KB page in the virtual address space to the same physical address.
 * If the page table for this address does not exist, it will be created.
 * @param physical_addr The physical address to map. The page containing this address will be mapped.
 */
void vmm_identity_map_page(physical_addr_t physical_addr);

/**
 * @brief Maps a physical address to a virtual address with the given flags.
 * @param virtual_addr The virtual address to map.
 * @param physical_addr The physical address to map to.
 * @param flags The flags for the page table entry.
 */
void vmm_map_page(virtual_addr_t virtual_addr, physical_addr_t physical_addr, kuint32_t flags);

/**
 * @brief Unmaps a virtual address.
 * @param virtual_addr The virtual address to unmap.
 */
void vmm_unmap_page(virtual_addr_t virtual_addr);

/**
 * @brief Gets the physical address that a virtual address is mapped to.
 * @param virtual_addr The virtual address to translate.
 * @return The physical address, or 0 if not mapped.
 */
physical_addr_t vmm_get_physical_addr(virtual_addr_t virtual_addr);

/**
 * @brief The handler for page faults (ISR 14).
 */
void page_fault_handler(struct registers *regs);

#endif //ARCH_I386_VMM_H
