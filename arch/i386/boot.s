#include "kernel_layout.h"

/* Declare constants for the multiboot header. */
.set ALIGN,     1<<0                        /* align loaded modules on page boundaries */
.set MEMINFO,   1<<1                        /* provide memory map */
.set VIDEO,     1<<2                        /* provide info about video mode */
.set FLAGS,    ALIGN | MEMINFO | VIDEO      /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002                   /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS)            /* checksum of above, to prove we are multiboot */

/* 
Declare the multiboot header based on the multiboot standard. 
Done in its own section so that it can be forced into the first 8KiB of
the Kernel file.
*/
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.skip 24
.long 0         // linear framebuffer
.long 1024      // width
.long 768       // height
.long 32        // bpp

/*
It is up to the kernel to provide a stack, this establishes the bottom of the 
stack section by creating a symbol for it, and then allocating 16KiB for the stack, 
and then creating another symbol for the top of the stack (due to x86 stack growing
downwards). Must be 16-byte aligned.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 #16KiB
stack_top:

/* 
Our linker script linker.ld specifies the _start as our entry point into the kernel;
this is where the bootloader will jump to once the kernel has been correctly loaded. 
Do not return from this function.
*/
.section .text
.global _start
.type _start, @function
_start:
    cli
    mov $stack_top, %esp

    pushl %ebx
    pushl %eax
    call kernel_main

1:  hlt
    jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start

