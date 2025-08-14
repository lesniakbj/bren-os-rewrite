#ifndef LIBC_SYSSTD_H
#define LIBC_SYSSTD_H

#include <libc/stdint.h>

// --- Process IPC/Control Syscalls ---
#define SYSCALL_PROC_YIELD      50
#define SYSCALL_PROC_EXIT       51
// #define SYSCALL_PROC_EXEC       50
#define SYSCALL_PROC_FORK       52
#define SYSCALL_PROC_WAIT       53
#define SYSCALL_PROC_WAIT_PID   54
#define SYSCALL_PROC_PID        55

void proc_yield();
void proc_exit(int status);
// void proc_exec(char *path);
void proc_fork();
void proc_wait();
void proc_wait_pid();
kuint32_t proc_pid();


// --- VFS (IO, File, Device) Syscalls ---
#define SYSCALL_VFS_WRITE       60

kint32_t vfs_write(kuint32_t fd, const char* buf, size_t count);


#endif //LIBC_SYSSTD_H
