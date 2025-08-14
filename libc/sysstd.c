#include <libc/sysstd.h>
#include <kernel/log.h>

void yield() {
    // We use the EAX register to tell the kernel which syscall we want.
    // 'int 0x80' is the traditional interrupt number for syscalls on x86.
    // By combining the move and the interrupt into a single assembly block,
    // we can prevent a context switch from happening between setting the
    // syscall number and invoking the interrupt, which was causing EAX to be
    // clobbered.
    asm volatile("movl %0, %%eax; int $0x80" : : "i" (SYSCALL_YIELD) : "eax");
}
