#include <arch/i386/memory.h>
#include <libc/strings.h>

physical_addr_t memsearch(const char *string, physical_addr_t startAddr, physical_addr_t endAddr) {
    // Get the size of the string we are searching for and validate address bounds
    size_t search_len = strlen(string);
    if(startAddr > endAddr || (endAddr - startAddr + 1) < search_len) {
        return 0;
    }

    // From startAddr -> endAddr, check to see if we match the characters in the string via a sliding window.
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

    // If we didn't find any matches we can return a 0 address.
    return 0;
}

physical_addr_t memsearch_aligned(const char *string, physical_addr_t startAddr, physical_addr_t endAddr, size_t alignment) {

    // Get the size of the string we are searching for and validate address bounds, validate alignment is power of 2
    size_t search_len = strlen(string);
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return 0;
    }
    if(startAddr > endAddr || (endAddr - startAddr + 1) < search_len) {
        return 0;
    }

    // Align start address to the specified boundary.
    physical_addr_t current_addr = (startAddr + alignment - 1) & ~(alignment - 1);

    // From startAddr -> endAddr, check to see if we match the characters in the string via a sliding window. Moves
    // only by the current alignment amount.
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

    // If we didn't find any matches we can return a 0 address.
    return 0;
}
