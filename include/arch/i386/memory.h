#ifndef _ARCH_I386_MEMORY_H
#define _ARCH_I386_MEMORY_H

#include <libc/stdint.h>

physical_addr_t memsearch(const char *string, physical_addr_t startAddr, physical_addr_t endAddr);
physical_addr_t memsearch_aligned(const char *string, physical_addr_t startAddr, physical_addr_t endAddr, size_t alignment);

#endif //_ARCH_I386_MEMORY_H
