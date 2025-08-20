#include <kernel/sync.h>

static inline bool interrupts_enabled() {
    kuint32_t flags;
    asm volatile (
        "pushfl \n"     // Push the EFLAGS register onto the stack
        "popl %0 \n"    // Pop the value from the stack into the 'flags' variable
        : "=r" (flags)  // Output operand: 'flags' is a general-purpose register
    );
    return (flags & (1 << 9)) != 0;
}

static bool restore_interrupts = false;

void spinlock_acquire(spinlock_t* lock) {
    if(interrupts_enabled()) {
        asm volatile("cli");
        restore_interrupts = true;
    }

    while (__sync_lock_test_and_set(lock, SPINLOCK_LOCKED) != 0)
        asm volatile("pause");
}

void spinlock_release(spinlock_t* lock) {
    __sync_lock_release(lock);

    if(restore_interrupts) {
        asm volatile("sti");
        restore_interrupts = false;
    }
}