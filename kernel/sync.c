#include <kernel/sync.h>

void spinlock_acquire(spinlock_t* lock) {
    asm volatile("cli");
    while (__sync_lock_test_and_set(lock, SPINLOCK_LOCKED) != 0) {
        asm volatile("pause");
    }
}

void spinlock_release(spinlock_t* lock) {
    __sync_lock_release(lock);
    asm volatile("sti");
}