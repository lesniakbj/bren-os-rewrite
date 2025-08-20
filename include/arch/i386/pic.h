#ifndef ARCH_I386_PIC_H
#define ARCH_I386_PIC_H

#include <libc/stdint.h>
#include <arch/i386/io.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01

#define PIC_EOI 0x20

void pic_remap(kuint32_t offset1, kuint32_t offset2);

kuint8_t pic_read_data_port(kuint16_t port);
kuint16_t pic_read_irr();
kuint16_t pic_read_isr();

#endif