#include <arch/i386/memory.h>
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
extern kuint32_t _kernel_start;
extern kuint32_t _kernel_end;

pmm_init_status_t pmm_init(multiboot_info_t *mbi) {
    pmm_init_status_t status;
    status.error = false;

    kuint32_t memory_size_kb = mbi->mem_lower + mbi->mem_upper;
    max_blocks = memory_size_kb * BYTES_PER_KB / PMM_BLOCK_SIZE;
    kuint32_t bitmap_size = (max_blocks + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    status.total_memory_kb = memory_size_kb;
    status.max_blocks = max_blocks;
    status.bitmap_size = bitmap_size;
    status.kernel_end = (kuint32_t)&_kernel_end;

    // Find a place for the memory map.
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    kuint32_t placement_address = 0;

    while((kuint32_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            kuint32_t region_start = mmap->addr;
            kuint32_t region_len = mmap->len;
            kuint32_t safe_start = ((kuint32_t)&_kernel_end + PMM_BLOCK_SIZE - 1) & ~(PMM_BLOCK_SIZE - 1);

            if (region_start > safe_start) {
                safe_start = region_start;
            }

            if (region_start + region_len > safe_start) {
                kuint32_t available_len = region_start + region_len - safe_start;
                if (available_len >= bitmap_size) {
                    placement_address = safe_start;
                    break;
                }
            }
        }
        mmap = (multiboot_memory_map_t*)((kuint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    if (placement_address == 0) {
        status.error = true;
        return status;
    }

    memory_map = (kuint32_t*)placement_address;
    status.placement_address = placement_address;

    // Mark all memory as used, then un-mark available regions.
    for (kuint32_t i = 0; i < max_blocks / PMM_BITS_PER_ENTRY; i++) memory_map[i] = PMM_ENTRY_FULL;

    mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    while((kuint32_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            for (kuint64_t i = 0; i < mmap->len; i += PMM_BLOCK_SIZE) {
                if (mmap->addr + i >= PMM_MANAGEABLE_MEMORY_START) {
                    pmm_clear_bit((mmap->addr + i) / PMM_BLOCK_SIZE);
                }
            }
        }
        mmap = (multiboot_memory_map_t*)((kuint32_t)mmap + mmap->size + sizeof(mmap->size));
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

void *pmm_alloc_block() {
    // Find the first free block.
    kint32_t frame = pmm_find_first_free();
    if (frame == PMM_NO_FREE_BLOCKS) {
        return 0; // Out of memory
    }

    // Mark the block as used.
    pmm_set_bit(frame);
    used_blocks++;

    // Calculate and return the physical address.
    return (void*)(frame * PMM_BLOCK_SIZE);
}

void pmm_free_block(void *p) {
    // Calculate the block index from the address.
    kuint32_t frame = (kuint32_t)p / PMM_BLOCK_SIZE;

    // Mark the block as free.
    pmm_clear_bit(frame);
    used_blocks--;
}

physical_addr_t memsearch(const char *string, physical_addr_t startAddr, physical_addr_t endAddr) {
    size_t search_len = strlen(string);

    if(startAddr > endAddr || (endAddr - startAddr + 1) < search_len) {
        return 0;
    }

    for (physical_addr_t current_addr = startAddr; current_addr <= endAddr - search_len + 1; ++current_addr) {
        bool found = true;
        for(size_t i = 0; i < search_len; ++i) {
            if(*((const char*)current_addr + i) != string[i]) {
                found = false;
                break;
            }
        }

        if(found) {
            return current_addr;
        }
    }

    return 0;
}

physical_addr_t memsearch_aligned(const char *string, physical_addr_t startAddr, physical_addr_t endAddr, size_t alignment) {
    size_t search_len = strlen(string);

    // Alignment must be a power of two and not zero.
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return 0; // Invalid alignment
    }

    if(startAddr > endAddr || (endAddr - startAddr + 1) < search_len) {
        return 0;
    }

    // Align start address to the specified boundary.
    physical_addr_t current_addr = (startAddr + alignment - 1) & ~(alignment - 1);

    for (; current_addr <= endAddr - search_len + 1; current_addr += alignment) {
        bool found = true;
        for(size_t i = 0; i < search_len; ++i) {
            if(*((const char*)current_addr + i) != string[i]) {
                found = false;
                break;
            }
        }

        if(found) {
            return current_addr;
        }
    }

    return 0;
}