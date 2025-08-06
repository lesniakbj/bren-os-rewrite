#pragma once

#if defined(__linux__)
#error "You are not using the correct cross compiler."
#endif

#if !defined(__i386__)
#error "Must target i386-elf."
#endif

#include <libc/stdint.h>
#include <libc/stdlib.h>

void kernel_main (kuint32_t magic, kuint32_t addr);

extern void enable_interrupts();