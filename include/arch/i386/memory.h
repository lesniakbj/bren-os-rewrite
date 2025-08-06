#ifndef ARCH_I386_MEMORY_H
#define ARCH_I386_MEMORY_H

#include <kernel/multiboot.h>
#include <libc/stdint.h>

// The size of a single physical memory block (page). 4KB is standard for x86.
#define PMM_BLOCK_SIZE 4096
#define BITS_PER_BYTE 8
#define BYTES_PER_KB 1024

// The number of bits in an entry of the memory map.
// Our memory map is an array of kuint32_t.
#define PMM_BITS_PER_ENTRY 32
#define PMM_ENTRY_FULL 0xFFFFFFFF

// The base address of memory we want to manage.
// We typically ignore the lower 1MB of physical memory as it's often
// reserved for BIOS, VGA, and other hardware.
#define PMM_MANAGEABLE_MEMORY_START 0x100000

// Return value from pmm_find_first_free when no free blocks are found.
#define PMM_NO_FREE_BLOCKS -1

// Structure to hold PMM initialization information for debugging.
typedef struct {
    kuint32_t total_memory_kb;
    kuint32_t max_blocks;
    kuint32_t bitmap_size;
    kuint32_t kernel_end;
    kuint32_t placement_address;
    kuint32_t used_blocks;
    bool error;
} pmm_init_status_t;

// Initializes the physical memory manager.
// This function should be called once, early in the kernel startup sequence.
// It uses the Multiboot information structure to determine available memory.
pmm_init_status_t pmm_init(multiboot_info_t *mbi);

// Allocates a single block (4KB) of physical memory.
// Returns a pointer to the start of the allocated block.
// Returns NULL if no free blocks are available.
void *pmm_alloc_block();

// Frees a previously allocated block of physical memory.
// 'p' must be a pointer to the start of a block that was allocated
// by pmm_alloc_block().
void pmm_free_block(void *p);

#endif // ARCH_I386_MEMORY_H