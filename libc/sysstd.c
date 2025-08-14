#include <libc/sysstd.h>
#include <kernel/log.h>

void proc_yield() {
    // We use the EAX register to tell the kernel which syscall we want.
    // 'int 0x80' is the traditional interrupt number for syscalls on x86.
    asm volatile("movl %0, %%eax; int $0x80" : : "i" (SYSCALL_PROC_YIELD) : "eax");
}

void proc_exit(int status) {
    // Use register constraints to load syscall number into eax and status into ebx.
    // This is cleaner and avoids the "impossible constraint" error.
    asm volatile("int $0x80" : : "a" (SYSCALL_PROC_EXIT), "b" (status));
}

kuint32_t proc_pid() {
    kuint32_t pid;
    asm volatile("int $0x80" : "=a" (pid) : "a" (SYSCALL_PROC_PID));
    return pid;
}

kint32_t vfs_write(kuint32_t fd, const char* buf, size_t count) {
    int bytes_written;
    asm volatile("int $0x80" :  "=a" (bytes_written) : "a" (SYSCALL_VFS_WRITE), "b" (fd), "c" (buf), "d"(count));
    return bytes_written;
}