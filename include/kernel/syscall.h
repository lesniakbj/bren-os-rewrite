#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <arch/i386/interrupts.h>

void syscall_handler(registers_t *regs);
void sys_yield(registers_t *regs);
void sys_exit(registers_t *regs);
void sys_pid(registers_t *regs);

void sys_vfs_write(registers_t *regs);

#endif
