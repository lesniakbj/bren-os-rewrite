#ifndef KERNEL_KERNEL_LAYOUT_H
#define KERNEL_KERNEL_LAYOUT_H

// TODO: This will be the future layout for the higher-half kernel, once we get there...
// Higher-half kernel virtual addresses
#define KERNEL_VIRTUAL_BASE     0xC0000000  // 3GB - Base of higher-half kernel
#define KERNEL_VIRTUAL_END      0xD0000000  // 3.5GB - End of kernel space

// Heap memory layout
#define HEAP_VIRTUAL_START      0xD0000000  // 3.25GB - Start of kernel heap
#define HEAP_SIZE               0x100000    // 1MB heap size

// Other kernel memory regions
#define KERNEL_STACK_SIZE       0x4000      // 16KB kernel stack

#endif // KERNEL_KERNEL_LAYOUT_H