#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/log.h>
#include <libc/sysstd.h>

void syscall_handler(registers_t *regs) {
    kuint32_t syscall = regs->eax;

    switch(syscall) {
        case SYSCALL_YIELD:
            sys_yield(regs);
            break;
        default:
            LOG_ERR("Uknown syscall: %d\n", syscall);
            break;
    }
}

void sys_yield(registers_t *regs) {
    proc_scheduler_run(regs);
}