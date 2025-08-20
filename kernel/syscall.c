#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/log.h>
#include <kernel/sync.h>
#include <libc/sysstd.h>

void syscall_handler(registers_t *regs) {
    kuint32_t syscall = regs->eax;

    switch(syscall) {
        case SYSCALL_PROC_YIELD:
            sys_yield(regs);
            break;
        case SYSCALL_PROC_EXIT:
            sys_exit(regs);
            break;
        case SYSCALL_PROC_PID:
            sys_pid(regs);
            break;
        case SYSCALL_VFS_WRITE:
            sys_vfs_write(regs);
            break;
        default:
            LOG_ERR("Unknown syscall: %d", syscall);
            break;
    }
}

void sys_yield(registers_t *regs) {
    proc_scheduler_run(regs);
}

void sys_exit(registers_t *regs) {
    process_t* proc = proc_get_current();
    LOG_INFO("Process %d has requested to exit with status: %d", proc->process_id, regs->ebx);
    proc_terminate(proc);
    proc_scheduler_run(regs);
}

void sys_pid(registers_t *regs) {
    LOG_INFO("Process has requested current PID");
    process_t *current_proc = proc_get_current();
    if (current_proc) {
        regs->eax = current_proc->process_id;
    } else {
        regs->eax = (kuint32_t)-1;
    }
}

void sys_vfs_write(registers_t *regs) {
    LOG_DEBUG("Entering VFS Write");

    kuint32_t fd = regs->ebx;
    const char* buf = (const char*)regs->ecx;
    size_t count = regs->edx;

    LOG_DEBUG("SYSCALL_VFS_WRITE: fd=%d, buf=0x%x, count=%d", fd, (kuint32_t)buf, count);

    process_t* proc = proc_get_current();
    file_node_t* node = proc->open_files[fd];
    if(node && node->write) {
        regs->eax = node->write(buf, count);
    } else {
        regs->eax = -1;
    }
    return;
}