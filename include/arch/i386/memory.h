#ifndef ARCH_I386_MEMORY_H
#define ARCH_I386_MEMORY_H

#include <kernel/multiboot.h>
#include <libc/stdint.h>

// The size of a single physical memory block (page). 4KB is standard for x86.
#define PMM_BLOCK_SIZE 4096

// Initializes the physical memory manager.
// This function should be called once, early in the kernel startup sequence.
// It uses the Multiboot information structure to determine available memory.
void pmm_init(multiboot_info_t *mbi);

// Allocates a single block (4KB) of physical memory.
// Returns a pointer to the start of the allocated block.
// Returns NULL if no free blocks are available.
void *pmm_alloc_block();

// Frees a previously allocated block of physical memory.
// 'p' must be a pointer to the start of a block that was allocated
// by pmm_alloc_block().
void pmm_free_block(void *p);

#endif // ARCH_I386_MEMORY_H