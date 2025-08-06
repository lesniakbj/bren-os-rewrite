#include <arch/i386/memory.h>
#include <kernel/multiboot.h>
#include <drivers/terminal.h> // For printing debug info

// We will use a static bitmap to track memory usage.
// The location and size will be determined by pmm_init.
static kuint32_t *memory_map = 0;
static kuint32_t max_blocks = 0;
static kuint32_t used_blocks = 0;

// Helper function to set a bit in the bitmap.
static void pmm_set_bit(kuint32_t bit) {
    memory_map[bit / 32] |= (1 << (bit % 32));
}

// Helper function to clear a bit in the bitmap.
static void pmm_clear_bit(kuint32_t bit) {
    memory_map[bit / 32] &= ~(1 << (bit % 32));
}

// Helper function to test if a bit is set.
static bool pmm_test_bit(kuint32_t bit) {
    return memory_map[bit / 32] & (1 << (bit % 32));
}

// Finds the first free block of memory and returns its index.
static kint32_t pmm_find_first_free() {
    for (kuint32_t i = 0; i < max_blocks / 32; i++) {
        // If the dword is all 1s, there are no free blocks in this chunk.
        if (memory_map[i] != 0xFFFFFFFF) {
            for (kuint32_t j = 0; j < 32; j++) {
                kuint32_t bit = 1 << j;
                if (!(memory_map[i] & bit)) {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1; // No free blocks found
}

// These are defined in linker.ld
extern kuint32_t _kernel_start;
extern kuint32_t _kernel_end;

void pmm_init(multiboot_info_t *mbi) {
    kuint32_t memory_size_kb = mbi->mem_lower + mbi->mem_upper;
    max_blocks = memory_size_kb * 1024 / PMM_BLOCK_SIZE;
    kuint32_t bitmap_size = max_blocks / 8;
    if (max_blocks % 8) bitmap_size++;

    terminal_writestring("--- PMM Initialization ---");
    terminal_writestringf("Total memory: %d KB (%d blocks)\n", memory_size_kb, max_blocks);
    terminal_writestringf("Required bitmap size: %d bytes\n", bitmap_size);
    terminal_writestringf("Kernel ends at: 0x%x\n", &_kernel_end);

    // Find a place for the memory map.
    // We must find a region of available memory that is:
    // 1. Marked as available by the bootloader.
    // 2. Large enough to hold our entire bitmap.
    // 3. Located safely after the end of our kernel code.
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    kuint32_t placement_address = 0;

    terminal_writestring("Memory map from bootloader:\n");
    while((kuint32_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        terminal_writestringf("  addr: 0x%x%x, len: 0x%x%x, type: %d\n",
            (kuint32_t)(mmap->addr >> 32), (kuint32_t)mmap->addr,
            (kuint32_t)(mmap->len >> 32), (kuint32_t)mmap->len,
            mmap->type);

        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            // This is a good candidate region. Does it have space for our bitmap
            // *after* the kernel?
            kuint32_t region_start = mmap->addr;
            kuint32_t region_len = mmap->len;
            
            // The earliest we can place our bitmap is after the kernel.
            kuint32_t safe_start = ((kuint32_t)&_kernel_end + 0xFFF) & ~0xFFF; // Align to 4K

            if (region_start > safe_start) {
                safe_start = region_start;
            }

            if (region_start + region_len > safe_start) {
                kuint32_t available_len = region_start + region_len - safe_start;
                if (available_len >= bitmap_size) {
                    placement_address = safe_start;
                    break; // Found a spot!
                }
            }
        }
        mmap = (multiboot_memory_map_t*)((kuint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    if (placement_address == 0) {
        terminal_writestring("Error: Could not find a suitable location for the memory map!\n");
        return;
    }

    memory_map = (kuint32_t*)placement_address;
    terminal_writestringf("Placing bitmap at: 0x%x\n", memory_map);

    // Initially, mark all memory as used.
    for (kuint32_t i = 0; i < max_blocks / 32; i++) memory_map[i] = 0xFFFFFFFF;

    // Now, un-mark the available memory regions based on the memory map.
    mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    while((kuint32_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            for (kuint64_t i = 0; i < mmap->len; i += PMM_BLOCK_SIZE) {
                if (mmap->addr + i >= 0x100000) { // Only manage memory above 1MB
                    pmm_clear_bit((mmap->addr + i) / PMM_BLOCK_SIZE);
                }
            }
        }
        mmap = (multiboot_memory_map_t*)((kuint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    // Finally, re-mark the memory used by the kernel and the bitmap itself as used.
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
    terminal_writestringf("%d blocks used initially.\n", used_blocks);
    terminal_writestring("PMM Initialized.\n");
}



void *pmm_alloc_block() {
    // Find the first free block.
    kint32_t frame = pmm_find_first_free();
    if (frame == -1) {
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
