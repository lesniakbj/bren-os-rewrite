#ifndef KERNEL_STD_LIB_H
#define KERNEL_STD_LIB_H

#include <libc/stdint.h>

// Vararg handling, stdarg.h style, let GCC handle the nastiness..
typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

// void *malloc(size_t size);
// void free(void *_Nullable ptr);
// void *calloc(size_t n, size_t size);
// void *realloc(void *_Nullable ptr, size_t size);
// void *reallocarray(void *_Nullable ptr, size_t n, size_t size);

void itoa(char* str_buf, int base, int value);

void* memcpy(void *destination, const void *source, size_t num);

#endif