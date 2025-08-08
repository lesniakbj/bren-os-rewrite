#ifndef STRINGS_H
#define STRINGS_H

#include <libc/stdint.h>

generic_ptr memset(generic_ptr bufptr, int value, size_t size);

size_t strlen(const char *str);
int strncmp(const char *s1, const char *s2, size_t n);

#endif //STRINGS_H
