#ifndef ARCH_I386_IO_H
#define ARCH_I386_IO_H

#include <libc/stdint.h>

extern void outb(kuint16_t port, kuint8_t value);
extern kuint8_t inb(kuint16_t port);
extern void outl(kuint16_t port, kuint32_t value);
extern kuint32_t inl(kuint16_t port);
extern void io_wait(void);

#endif