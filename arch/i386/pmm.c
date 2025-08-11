#include <arch/i386/pmm.h>
#include <kernel/multiboot.h>

// We will use a static bitmap to track memory usage.
// The location and size will be determined by pmm_init.
static kuint32_t *memory_map = 0;
static kuint32_t max_blocks = 0;
static kuint32_t used_blocks = 0;

// Helper function to set a bit in the bitmap.
static void pmm_set_bit(kuint32_t bit) {
    memory_map[bit / PMM_BITS_PER_ENTRY] |= (1 << (bit % PMM_BITS_PER_ENTRY));
}

// Helper function to clear a bit in the bitmap.
static void pmm_clear_bit(kuint32_t bit) {
    memory_map[bit / PMM_BITS_PER_ENTRY] &= ~(1 << (bit % PMM_BITS_PER_ENTRY));
}

// Helper function to test if a bit is set.
static bool pmm_test_bit(kuint32_t bit) {
    return memory_map[bit / PMM_BITS_PER_ENTRY] & (1 << (bit % PMM_BITS_PER_ENTRY));
}

// Finds the first free block of memory and returns its index.
static kint32_t pmm_find_first_free() {
    for (kuint32_t i = 0; i < max_blocks / PMM_BITS_PER_ENTRY; i++) {
        // If the dword is all 1s, there are no free blocks in this chunk.
        if (memory_map[i] != PMM_ENTRY_FULL) {
            // Use the bit index to find exactly which of the 32 memory blocks represented by that integer is the first one that's free
            for (kuint32_t j = 0; j < PMM_BITS_PER_ENTRY; j++) {
                kuint32_t bit = 1 << j;
                if (!(memory_map[i] & bit)) {
                    return i * PMM_BITS_PER_ENTRY + j;
                }
            }
        }
    }
    return PMM_NO_FREE_BLOCKS; // No free blocks found
}

// These are defined in linker.ld
extern physical_addr_t _kernel_start;
extern physical_addr_t _kernel_end;

pmm_init_status_t pmm_init(multiboot_info_t *mbi) {
    pmm_init_status_t status;
    status.error = false;

    // Using the multiboot info, calculate the size of memory/# of blocks we have.
    // Then use that to determine the bitmap size we need to store those blocks.
    kuint32_t memory_size_kb = mbi->mem_lower + mbi->mem_upper;
    max_blocks = memory_size_kb * BYTES_PER_KB / PMM_BLOCK_SIZE;
    kuint32_t bitmap_size = (max_blocks + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    status.total_memory_kb = memory_size_kb;
    status.max_blocks = max_blocks;
    status.bitmap_size = bitmap_size;
    status.kernel_end = (physical_addr_t)&_kernel_end;

    // Find a place for the memory map.
    // The bootloader (GRUB) provides a map of the system's memory layout. We need to iterate through this
    // map to find a region of available memory that is large enough to hold our PMM bitmap.
    // This bitmap is essential for tracking which parts of memory are free or in use.
    // Note: At this stage, paging is not enabled, so virtual addresses are the same as physical addresses.
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    physical_addr_t placement_address = 0;

    // Loop through each entry in the memory map provided by the bootloader.
    while((physical_addr_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        // We are only interested in regions that are marked as available for use.
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            physical_addr_t region_start = mmap->addr;
            kuint32_t region_len = mmap->len;

            // Calculate the first possible safe address to place our bitmap.
            // This must be after the kernel's code and data, and aligned to a block boundary.
            physical_addr_t safe_start = ((physical_addr_t)&_kernel_end + PMM_BLOCK_SIZE - 1) & ~(PMM_BLOCK_SIZE - 1);

            // If the available region starts after our calculated safe_start, then we should
            // consider placing the bitmap at the beginning of this region instead.
            if (region_start > safe_start) {
                safe_start = region_start;
            }

            // Check if there is any usable space in this region beyond the safe_start address.
            if (region_start + region_len > safe_start) {
                // Calculate how much space is actually available in this chunk.
                kuint32_t available_len = region_start + region_len - safe_start;

                // If there's enough space for our bitmap, we've found our spot.
                if (available_len >= bitmap_size) {
                    placement_address = safe_start;
                    break; // Exit the loop, as we've found a suitable location.
                }
            }
        }

        // The multiboot memory map entries are variable in size.
        // The 'size' field indicates the size of the entry *not including* the 'size' field itself.
        // So, we advance the pointer by the value of 'size' plus the size of the 'size' field.
        mmap = (multiboot_memory_map_t*)((physical_addr_t)mmap + mmap->size + sizeof(mmap->size));
    }

    // If we didn't find a place to store our memory map, set the error status and return
    if (placement_address == 0) {
        status.error = true;
        return status;
    }

    // Set our bitmap memory block tracking to the placement address location we found
    memory_map = (physical_addr_t*)placement_address;
    status.placement_address = placement_address;

    // Initially, mark all memory blocks in the bitmap as used.
    for (kuint32_t i = 0; i < max_blocks / PMM_BITS_PER_ENTRY; i++) {
        memory_map[i] = PMM_ENTRY_FULL;
    }

    // Second pass over the memory map. This time, we're initializing the bitmap.
    mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    while((physical_addr_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        // We are only interested in regions that are marked as available for use.
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            // The Multiboot spec provides 64-bit addresses and lengths, even for 32-bit systems,
            // to support features like PAE. Our PMM is currently 32-bit, so we must handle this carefully.
            kuint64_t region_addr = mmap->addr;
            kuint64_t region_len = mmap->len;

            // Iterate through the memory region in block-sized steps.
            for (kuint64_t i = 0; i < region_len; i += PMM_BLOCK_SIZE) {
                kuint64_t current_addr = region_addr + i;

                // We only manage memory starting from a specific address (PMM_MANAGEABLE_MEMORY_START),
                // which is typically 1MB, to avoid dealing with legacy hardware regions.
                // Crucially, we also must skip any memory that is beyond our PMM's 32-bit (4GB) limit.
                if (current_addr >= PMM_MANAGEABLE_MEMORY_START && current_addr < 0x100000000) {
                    // Clear the bit for this block in the bitmap, marking it as free.
                    // The cast to physical_addr_t (kuint32_t) is now safe.
                    pmm_clear_bit((physical_addr_t)(current_addr / PMM_BLOCK_SIZE));
                }
            }
        }
        // Advance to the next entry in the memory map.
        mmap = (multiboot_memory_map_t*)((physical_addr_t)mmap + mmap->size + sizeof(mmap->size));
    }

    // Re-mark the kernel and bitmap as used.
    kuint32_t kernel_start_block = (kuint32_t)&_kernel_start / PMM_BLOCK_SIZE;
    kuint32_t kernel_end_block = ((kuint32_t)&_kernel_end + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
    for (kuint32_t i = kernel_start_block; i <= kernel_end_block; i++) {
        pmm_set_bit(i);
    }

    kuint32_t bitmap_block_start = (kuint32_t)memory_map / PMM_BLOCK_SIZE;
    kuint32_t bitmap_block_end = ((kuint32_t)memory_map + bitmap_size) / PMM_BLOCK_SIZE;
    for (kuint32_t i = bitmap_block_start; i <= bitmap_block_end; i++) {
        pmm_set_bit(i);
    }

    used_blocks = 0;
    for (kuint32_t i = 0; i < max_blocks; i++) {
        if (pmm_test_bit(i)) used_blocks++;
    }
    status.used_blocks = used_blocks;

    return status;
}

generic_ptr pmm_alloc_block() {
    // Find the first free block.
    kint32_t frame = pmm_find_first_free();
    if (frame == PMM_NO_FREE_BLOCKS) {
        return 0; // Out of memory
    }

    // Mark the block as used.
    pmm_set_bit(frame);
    used_blocks++;

    // Calculate and return the physical address.
    return (generic_ptr)(frame * PMM_BLOCK_SIZE);
}

void pmm_free_block(generic_ptr p) {
    // Calculate the block index from the address.
    kuint32_t frame = (physical_addr_t)p / PMM_BLOCK_SIZE;

    // Mark the block as free.
    pmm_clear_bit(frame);
    used_blocks--;
}
