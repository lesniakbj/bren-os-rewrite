#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>
#include <arch/i386/vmm.h>

#define STOPPED 0
#define INIT    1
#define RUNNING 2
#define KILLED  3
#define PAUSED  4

typedef struct process {
    registers_t *registers;
    kuint32_t process_id;
    kuint8_t current_state;
    kuint32_t *page_directory;
    virtual_addr_t *kernel_stack;
    size_t kernel_stack_size;
    kuint32_t parent_proc_id;
} process_t;

process_t* proc_create();
void proc_scheduler_run();

#endif