#ifndef LIB_STRINGS_H
#define LIB_STRINGS_H

#include <libc/stdint.h>
#include <libc/stdlib.h>

generic_ptr memset(generic_ptr bufptr, kint32_t value, size_t size);
generic_ptr memcpy(generic_ptr destination, const generic_ptr source, size_t num);

size_t strlen(const char *str);
kint32_t strncmp(const char *s1, const char *s2, size_t n);
char* strrchr(const char* str, kint32_t c);

kint32_t sprintf(char* buffer, const char* format, ...);
kint32_t snprintf(char* buffer, size_t buff_size, const char* format, ...);

#endif
