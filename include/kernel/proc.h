#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>
#include <arch/i386/vmm.h>

#define MAX_PROCESSES 1024
#define KERNEL_STACK_SIZE 4096

#define STOPPED 0
#define RUNNING 1
#define KILLED  2
#define PAUSED  3

typedef struct process {
    kuint32_t esp; /* Saved ESP */
    kuint32_t process_id;
    kuint8_t current_state;
    kuint32_t *page_directory;
    generic_ptr kernel_stack;
    size_t kernel_stack_size;
    kuint32_t parent_proc_id;
    bool used;
} process_t;

typedef void (*proc_entry_point_t)(void);

int proc_init();
process_t* proc_create(proc_entry_point_t entry_point, bool restore_interrupts);
void proc_scheduler_run(registers_t *regs);

#endif