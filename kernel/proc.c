#include <kernel/proc.h>
#include <kernel/heap.h>
#include <libc/strings.h>
#include <arch/i386/vmm.h>
#include <kernel/log.h>

static process_t process_table[MAX_PROCESSES];
static kuint32_t next_pid = 1;
static kuint32_t current_process_index = 0;
static bool init_done = false;

int proc_init() {
    LOG_DEBUG("PROC: Initializing process table...\n");
    for(int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].used = false;
        process_table[i].current_state = STOPPED;
    }
    // The kernel's main thread (PID 0) is the initial process and will become the idle task.
    process_table[0].used = true;
    process_table[0].current_state = RUNNING;
    process_table[0].process_id = 0;
    process_table[0].esp = 0; // ESP is managed by the context switch, will be set on first switch away from kernel. 
    current_process_index = 0;
    init_done = true;
    LOG_DEBUG("PROC: Initialization complete.\n");
    return 0;
}

process_t* proc_create(proc_entry_point_t entry_point, bool restore_interrupts) {
    asm volatile("cli"); // Disable interrupts for this critical section

    if(!init_done) {
        LOG_ERR("PROC: proc_create called before proc_init!\n");
        if(restore_interrupts) {
            asm volatile("sti"); // Re-enable interrupts before returning
        }
        return NULL;
    }

    process_t* new_proc = NULL;
    int new_proc_idx = -1;
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].used == false) {
            new_proc = &process_table[i];
            new_proc_idx = i;
            break;
        }
    }

    if (new_proc == NULL) {
        LOG_ERR("PROC: No free process slots available.\n");
        if(restore_interrupts) {
            asm volatile("sti"); // Re-enable interrupts before returning
        }
        return NULL;
    }

    LOG_DEBUG("PROC: Creating new process in slot %d.\n", new_proc_idx);

    // Allocate a kernel stack
    new_proc->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    if (new_proc->kernel_stack == NULL) {
        LOG_ERR("PROC: Failed to allocate kernel stack for new process.\n");
        if(restore_interrupts) {
            asm volatile("sti"); // Re-enable interrupts before returning
        }
        return NULL;
    }

    new_proc->kernel_stack_size = KERNEL_STACK_SIZE;
    LOG_DEBUG("PROC: Kernel stack allocated at 0x%x.\n", new_proc->kernel_stack);

    // Set up the initial stack frame for the new process.
    char* stack_ptr_char = (char*)new_proc->kernel_stack + KERNEL_STACK_SIZE;
    stack_ptr_char -= sizeof(registers_t);
    registers_t* regs = (registers_t*)stack_ptr_char;

    memset(regs, 0, sizeof(registers_t));

    regs->eip = (kuint32_t)entry_point;
    regs->cs = 0x08; // Kernel code segment
    regs->eflags = 0x202; // Interrupts enabled
    regs->gs = 0x10; // Kernel data segment
    regs->fs = 0x10;
    regs->es = 0x10;
    regs->ds = 0x10;

    new_proc->esp = (kuint32_t)regs;
    LOG_DEBUG("PROC: Initial stack frame created at 0x%x.\n", new_proc->esp);

    new_proc->process_id = next_pid++;
    new_proc->parent_proc_id = process_table[current_process_index].process_id;
    new_proc->current_state = RUNNING;
    new_proc->used = true;
    new_proc->page_directory = vmm_get_kernel_directory();

    LOG_DEBUG("PROC: Process %d created successfully.\n", new_proc->process_id);

    if(restore_interrupts) {
        asm volatile("sti"); // Re-enable interrupts before returning
    }
    return new_proc;
}

void proc_scheduler_run(registers_t *regs) {
    if(!init_done) {
        return;
    }

    kuint32_t last_process_index = current_process_index;

    // Simple round-robin scheduler
    kuint32_t next_process_index = last_process_index;
    int attempts = 0;
    do {
        next_process_index = (next_process_index + 1) % MAX_PROCESSES;
        if (process_table[next_process_index].used && process_table[next_process_index].current_state == RUNNING) {
            break; // Found a runnable process
        }
        attempts++;
    } while(attempts < MAX_PROCESSES);

    // If no other runnable process was found, stay on the current one.
    if (next_process_index == last_process_index) {
        return;
    }

    // Save the register state pointer of the current process.
    process_table[last_process_index].esp = (kuint32_t)regs;

    // Get the register state pointer for the next process.
    kuint32_t next_proc_esp = process_table[next_process_index].esp;

    // Update the current process index.
    current_process_index = next_process_index;

    // Perform the context switch.
    context_switch(next_proc_esp);
}
