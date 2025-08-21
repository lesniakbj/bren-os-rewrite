#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <arch/i386/interrupts.h>
#include <libc/sysstd.h>

void syscall_handler(registers_t *regs);
void sys_yield(registers_t *regs);
void sys_exit(registers_t *regs);
void sys_pid(registers_t *regs);

void sys_vfs_write(registers_t *regs);

// Safe user memory access
bool validate_user_pointer(const void* ptr, size_t size);
bool copy_from_user(void* kdest, const void* usrc, size_t size);

#endif // KERNEL_SYSCALL_H