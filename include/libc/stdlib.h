#ifndef KERNEL_STD_LIB_H
#define KERNEL_STD_LIB_H

#include <libc/stdint.h>

// Vararg handling, let GCC handle the nastiness..
typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

void itoa(char* str_buf, kint32_t base, kint32_t value);

#endif