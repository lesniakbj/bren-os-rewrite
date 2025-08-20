#ifndef KERNEL_SYNC_H
#define KERNEL_SYNC_H

#define SPINLOCK_LOCKED 0
#define SPINLOCK_UNLOCKED 1

#include <libc/stdint.h>

typedef volatile kuint32_t spinlock_t;

void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);

#endif