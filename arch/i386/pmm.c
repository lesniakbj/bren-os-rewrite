#include <arch/i386/pmm.h>
#include <kernel/multiboot.h>
#include <kernel/kernel_layout.h>

// We will use a static bitmap to track memory usage.
// The location and size will be determined by pmm_init.
static kuint32_t *memory_map = 0;
static kuint32_t max_blocks = 0;
static kuint32_t used_blocks = 0;

// These are defined in linker.ld
extern physical_addr_t _kernel_start_phys;
extern physical_addr_t _kernel_end_phys;

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

// Helper to find the first free block of memory and returns its index.
static kint32_t pmm_find_first_free() {
    for (kuint32_t i = 0; i < max_blocks / PMM_BITS_PER_ENTRY; i++) {
        // If the dword is all 1's, there are no free blocks in this chunk.
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

pmm_init_status_t pmm_init(multiboot_info_t *mbi) {
    pmm_init_status_t status = {0};
    status.error = false;

    // --- Step 0: compute max blocks and bitmap size ---
    kuint32_t memory_size_kb = mbi->mem_lower + mbi->mem_upper;
    max_blocks = memory_size_kb * BYTES_PER_KB / PMM_BLOCK_SIZE;
    size_t bitmap_size = (max_blocks + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    status.total_memory_kb = memory_size_kb;
    status.max_blocks = max_blocks;
    status.bitmap_size = bitmap_size;
    status.kernel_start = (physical_addr_t)&_kernel_start_phys;
    status.kernel_end   = (physical_addr_t)&_kernel_end_phys;

    // --- Step 1: find a region for the bitmap ---
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    physical_addr_t placement_address = 0;

    while((physical_addr_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            physical_addr_t safe_start = ((physical_addr_t)&_kernel_end_phys + PMM_BLOCK_SIZE - 1) & ~(PMM_BLOCK_SIZE - 1);
            if (mmap->addr > safe_start) safe_start = mmap->addr;

            if (mmap->addr + mmap->len > safe_start && mmap->len >= bitmap_size) {
                placement_address = safe_start;
                break;
            }
        }
        mmap = (multiboot_memory_map_t*)((physical_addr_t)mmap + mmap->size + sizeof(mmap->size));
    }

    if (!placement_address) { status.error = true; return status; }

    memory_map = (physical_addr_t*)(placement_address + KERNEL_VIRTUAL_BASE);
    status.placement_address = placement_address;

    // --- Step 2: mark all blocks used ---
    for (kuint32_t i = 0; i < max_blocks / PMM_BITS_PER_ENTRY; i++) memory_map[i] = PMM_ENTRY_FULL;

    // --- Step 3: clear usable blocks from the memory map ---
    mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    while((physical_addr_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            kuint64_t start = mmap->addr;
            kuint64_t end   = mmap->addr + mmap->len;
            if (start < (physical_addr_t)&_kernel_end_phys) start = (physical_addr_t)&_kernel_end_phys;
            for (kuint64_t addr = start; addr + PMM_BLOCK_SIZE <= end; addr += PMM_BLOCK_SIZE) {
                if (addr < 0x100000000) pmm_clear_bit((physical_addr_t)(addr / PMM_BLOCK_SIZE));
            }
        }
        mmap = (multiboot_memory_map_t*)((physical_addr_t)mmap + mmap->size + sizeof(mmap->size));
    }

    // --- Step 4: re-mark kernel & bitmap as used ---
    for (kuint32_t i = (kuint32_t)&_kernel_start_phys / PMM_BLOCK_SIZE;
         i <= ((kuint32_t)&_kernel_end_phys + PMM_BLOCK_SIZE - 1)/PMM_BLOCK_SIZE; i++) pmm_set_bit(i);

    for (kuint32_t i = (kuint32_t)memory_map / PMM_BLOCK_SIZE;
         i <= ((kuint32_t)memory_map + bitmap_size)/PMM_BLOCK_SIZE; i++) pmm_set_bit(i);

    // --- Step 5: count used blocks ---
    used_blocks = 0;
    for (kuint32_t i = 0; i < max_blocks; i++) if (pmm_test_bit(i)) used_blocks++;
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
