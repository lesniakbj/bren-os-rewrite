#include <arch/i386/memory.h>
#include <libc/strings.h>

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
