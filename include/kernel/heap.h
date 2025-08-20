#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#define HEAP_MAGIC 0x12345678
#define HEAP_MIN_SIZE 0x10000    // 64KB minimum heap size
#define FIXED_MIN_BLOCK_SIZE

#ifndef FIXED_MIN_BLOCK_SIZE
#define MIN_BLOCK_SIZE 128
#endif

#include <libc/stdint.h>
#include <kernel/kernel_layout.h>
#include <arch/i386/vmm.h>

typedef struct heap_block {
    size_t size;                 // Size of the block (including this header)
    struct heap_block* next;     // Pointer to the next block
    struct heap_block* prev;     // Pointer to the prev block
    int free;                    // 1 if free, 0 if used
    kuint32_t magic;             // Magic number for validation (HEAP_MAGIC)
} heap_block_t;

void heap_init(virtual_addr_t start, size_t size);
generic_ptr kmalloc(size_t size);
void kfree(generic_ptr ptr);
generic_ptr krealloc(generic_ptr ptr, size_t size);
bool heap_expand(size_t additional_size);

size_t heap_get_total_size();
size_t heap_get_free_size();
size_t heap_get_used_size();

#endif