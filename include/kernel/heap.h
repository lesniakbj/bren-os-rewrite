#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <libc/stdint.h>

/**
 * @brief Initializes the kernel heap.
 * Sets up the heap allocator with the specified start address and size.
 * 
 * @param start The virtual address where the heap should start
 * @param size The size of the heap in bytes
 */
void heap_init(virtual_addr_t start, size_t size);

/**
 * @brief Allocates a block of memory of the specified size.
 * 
 * @param size The size of the block to allocate in bytes
 * @return A pointer to the allocated block, or NULL if allocation failed
 */
generic_ptr kmalloc(size_t size);

/**
 * @brief Frees a previously allocated block of memory.
 * 
 * @param ptr A pointer to the block to free
 */
void kfree(generic_ptr ptr);

/**
 * @brief Reallocates a block of memory to a new size.
 * 
 * @param ptr A pointer to the block to reallocate
 * @param size The new size for the block
 * @return A pointer to the reallocated block, or NULL if reallocation failed
 */
generic_ptr krealloc(generic_ptr ptr, size_t size);

#endif // KERNEL_HEAP_H