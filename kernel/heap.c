#include <kernel/heap.h>
#include <arch/i386/pmm.h>
#include <arch/i386/vmm.h>
#include <drivers/terminal.h>
#include <libc/strings.h>
#include <libc/stdint.h>

static heap_block_t* heap_start = NULL;
static virtual_addr_t heap_virtual_start = 0;
static size_t heap_size = 0;

void heap_init(virtual_addr_t start, size_t size) {
    // STEP 1: Align the start address to a page boundary (use bitwise AND with 0xFFFFF000)
    virtual_addr_t aligned_addr = start & 0xFFFFF000;

    // STEP 2: Ensure minimum heap size (use HEAP_MIN_SIZE constant)
    if(size < HEAP_MIN_SIZE) {
        size = HEAP_MIN_SIZE;
    }
    
    // STEP 3: Align heap size to page boundary
    size_t aligned_size = (size + 0xFFF) & 0xFFFFF000;

    // STEP 4: Map pages for the heap using vmm_map_page()
    //   - Loop through each 4KB page
    //   - Allocate physical memory with pmm_alloc_block()
    //   - Map virtual to physical addresses with proper flags (PTE_PRESENT | PTE_READ_WRITE)
    for(kuint32_t offset = 0; offset < aligned_size; offset += PAGE_SIZE) {
        virtual_addr_t addr = aligned_addr + offset;
        physical_addr_t block = pmm_alloc_block();
        if(!block) {
            terminal_writestring("HEAP Error: Failed to allocate physical memory for heap\n");
            return;
        }
        vmm_map_page(addr, block, (PTE_PRESENT | PTE_READ_WRITE));
    }
    
    // STEP 5: Initialize the first block that represents the entire heap
    //   - Set size to heap_size
    //   - Set next to NULL
    //   - Set prev to NULL
    //   - Set free to 1 (available)
    //   - Set magic to HEAP_MAGIC
    heap_start = (heap_block_t*)aligned_addr;
    heap_start->size = aligned_size;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    heap_start->free = 1;
    heap_start->magic = HEAP_MAGIC;

    // STEP 6: Store heap_start, heap_virtual_start, and heap_size in global variables
    heap_virtual_start = heap_start;
    heap_size = aligned_size;
    terminal_writestringf("Heap initialized at 0x%x with size 0x%x\n", heap_virtual_start, heap_size);
}

generic_ptr kmalloc(size_t size) {
    // STEP 1: Check if heap is initialized (heap_start != NULL)
    if(heap_start == NULL) {
        terminal_writestring("HEAP Error: Heap not initialized, you must initialize the heap before allocating\n");
        return NULL;
    }
    
    // STEP 2: Calculate total size needed (requested size + header size)
    size_t total_size = size + sizeof(heap_block_t);
    
    // STEP 3: Align up total size to 4-byte boundary
    size_t aligned_size = (total_size + 3) & ~3;
    
    // STEP 4: Find a free block using first-fit algorithm:
    //   - Start at heap_start
    //   - Traverse the linked list of blocks heap_block_t
    //   - Look for a free block (free == 1) that's large enough
    heap_block_t* cur_block = heap_start;
    while (cur_block != NULL) {
        if (cur_block->magic != HEAP_MAGIC) {
            terminal_writestring("HEAP Error: Heap corruption detected during traversal!\n");
            return NULL;
        }

        if (cur_block->free == 1 && cur_block->size >= aligned_size) {
            break;
        }
        cur_block = cur_block->next;
    }

    // Time to expand the heap...
    if(cur_block == NULL) {
        size_t expansion_needed = aligned_size > heap_size / 4 ? aligned_size : heap_size / 4;
        if (heap_expand(expansion_needed)) {
            // Reattempt allocation...
            heap_block_t* cur_block = heap_start;
            while (cur_block != NULL) {
                if (cur_block->magic != HEAP_MAGIC) {
                    terminal_writestring("HEAP Error: Heap corruption detected during traversal!\n");
                    return NULL;
                }

                if (cur_block->free == 1 && cur_block->size >= aligned_size) {
                    break;
                }
                cur_block = cur_block->next;
            }
        } else {
            terminal_writestringf("HEAP Error: No suitable block found and expansion failed for size: %d\n", aligned_size);
            return NULL;
        }
        terminal_writestringf("HEAP Error: No suitable block found for size: %d\n", aligned_size);
        return NULL;
    }

    // Also return early if the current block doesn't have the correct magic number
    if(cur_block->magic != HEAP_MAGIC) {
        terminal_writestring("HEAP Error: Block corruption detected!\n");
        return NULL;
    }

    // STEP 5: If found block is much larger than needed, split it:
    //  - Create a new block after the allocated space
    //  - Set its size, next pointer, free flag, and magic
    //  - Update the original block's size and next pointer
#ifdef FIXED_MIN_BLOCK_SIZE
    // Only split if we can reclaim at least 25% of the original block
    if(cur_block->size >= aligned_size + (cur_block->size / 4)) {
#else
     // Only split if it would reclaim more than 128 bytes
    if(cur_block->size >= aligned_size + MIN_BLOCK_SIZE) {
#endif
        // Address of new block
        heap_block_t* new_block = (heap_block_t*)((virtual_addr_t)cur_block + aligned_size);

        // Initialize new block
        new_block->size = cur_block->size - aligned_size;
        new_block->next = cur_block->next;
        new_block->prev = cur_block;
        new_block->free = 1;
        new_block->magic = HEAP_MAGIC;
        if (new_block->next) {
            new_block->next->prev = new_block;
        }

        // Update original block
        cur_block->size = aligned_size;
        cur_block->next = new_block;

        // Validate blocks to ensure correct splitting
        if(cur_block->magic != HEAP_MAGIC || new_block->magic != HEAP_MAGIC) {
            terminal_writestring("HEAP Error: Block corruption detected after split!\n");
            return NULL;
        }
    }


    // STEP 6: Mark the found block as used (free = 0)
    cur_block->free = 0;
    
    // STEP 7: Return pointer to memory after the header
    return (generic_ptr)((virtual_addr_t)cur_block + sizeof(heap_block_t));
}

void kfree(generic_ptr ptr) {
    // STEP 1: Handle NULL pointer (just return)
    if (ptr == NULL) {
        return;
    }

    // STEP 2: Get the block header by subtracting header size from ptr
    heap_block_t* block = (heap_block_t*)((virtual_addr_t)ptr - sizeof(heap_block_t));

    // STEP 3: Validate the block using magic number
    if (block->magic != HEAP_MAGIC) {
        terminal_writestring("HEAP Error: Invalid block header in kfree!\n");
        return;
    }

    // STEP 4: Mark the block as free (free = 1)
    block->free = 1;
    
    // STEP 5: Coalesce with next block if it's free:
    //   - Check if next block exists and is free
    //   - Check if blocks are contiguous in memory
    //   - If so, merge them by adding sizes and updating next pointer
    if(block->next != NULL && block->next->free == 1) {
        virtual_addr_t block_end_addr = (virtual_addr_t)block + block->size;
        if(block_end_addr == (virtual_addr_t)block->next) {
            block->size = block->size + block->next->size;
            block->next = block->next->next;
            if (block->next) {
                 block->next->prev = block;
            }
        }
    }
    
    // STEP 6: Coalesce with previous block if it's free:
    //   - Check if previous block is free and contiguous
    //   - If so, merge them
    if (block->prev && block->prev->free) {
        virtual_addr_t block_prev_end_addr = (virtual_addr_t)block->prev + block->prev->size;
        if(block_prev_end_addr == (virtual_addr_t)block) {
            block->prev->size += block->size;
            block->prev->next = block->next;
            if (block->next) {
                  block->prev = block;
            }
            block = block->prev;
        }
    }
}

generic_ptr krealloc(generic_ptr ptr, size_t size) {
    // STEP 1: Handle special cases:
    //   - If ptr is NULL, behave like kmalloc(size)
    //   - If size is 0, behave like kfree(ptr)
    if(ptr == NULL) {
        return kmalloc(size);
    } else if(size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // STEP 2: Get the block header and validate it
    heap_block_t* block = (heap_block_t*)((virtual_addr_t)ptr - sizeof(heap_block_t));
    if(block->magic != HEAP_MAGIC) {
        terminal_writestring("HEAP Error: Invalid block header in krealloc!\n");
        return;
    }
    
    // STEP 3: Calculate current payload size (block size - header size)
    size_t payload_size = block->size - sizeof(heap_block_t);

    // STEP 4: If new size is smaller or equal to current size, return same pointer
    if(payload_size <= block->size) {
        return ptr;
    }
    
    // STEP 5: Otherwise:
    //   - Allocate new block with kmalloc(size)
    //   - Copy data from old block to new block using memcpy
    //   - Free old block with kfree
    //   - Return new pointer
    generic_ptr new_block = kmalloc(size);
    if(new_block) {
        size_t copy_size = payload_size < size ? payload_size : size;
        memcpy(new_block, block, copy_size);
        kfree(block);
    }
    return new_block;
}

bool heap_expand(size_t additional_size) {
    // Calculate how many new pages needed
    size_t pages_needed = (additional_size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t expansion_size = pages_needed * PAGE_SIZE;

    // Find the last block
    heap_block_t* last_block = heap_start;
    while (last_block && last_block->next) {
        last_block = last_block->next;
    }

    // Get the end address of the current heap
    virtual_addr_t current_heap_end = heap_virtual_start + heap_size;

    // Map all the new pages
    for (size_t i = 0; i < expansion_size; i += PAGE_SIZE) {
        virtual_addr_t addr = current_heap_end + i;
        physical_addr_t block = pmm_alloc_block();
        if(!block) {
            terminal_writestring("HEAP Error: Failed to allocate physical memory for heap in expansion\n");
            return false;
        }
        vmm_map_page(addr, block, (PTE_PRESENT | PTE_READ_WRITE));
    }

    // Create a new free block at the end of the expanded heap
    heap_block_t* new_block = (heap_block_t*)current_heap_end;
    new_block->size = expansion_size;
    new_block->next = NULL;
    new_block->prev = last_block;
    new_block->free = 1;
    new_block->magic = HEAP_MAGIC;

    if(last_block) {
        last_block->next = new_block;
    }

    // Update heap size
    heap_size += expansion_size;

    // Coalesce with previous block if it is free
    if(last_block && last_block->free) {
        virtual_addr_t last_block_end = (virtual_addr_t)last_block + last_block->size;
        if(last_block_end == (virtual_addr_t)new_block) {
            last_block->size += new_block->size;
            last_block->next = NULL;
        }
    }

    return true;
}


size_t heap_get_total_size() {
    return heap_size;
}

size_t heap_get_free_size() {
    return heap_size - heap_get_used_size();
}

size_t heap_get_used_size() {
    if (!heap_start) return 0;

    size_t used = 0;
    heap_block_t* cur = heap_start;
    while(cur != NULL) {
        if(cur->magic == HEAP_MAGIC && cur->free == 0) {
            used += cur->size;
        }
        cur = cur->next;
    }

    return used;
}