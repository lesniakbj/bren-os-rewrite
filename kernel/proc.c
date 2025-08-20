#include <kernel/proc.h>
#include <kernel/heap.h>
#include <kernel/log.h>
#include <kernel/vfs.h>
#include <libc/strings.h>
#include <arch/i386/vmm.h>
#include <arch/i386/gdt.h>
#include <arch/i386/pmm.h>

static process_t process_table[MAX_PROCESSES];
static kuint32_t next_pid = 1;
static kuint32_t current_process_index = 0;
static bool init_done = false;

static const char* proc_type_names[] = {
    [KERNEL_PROC] = "Kernel Process",
    [USER_PROC] = "User Process",
    [DAEMON] = "Daemon Process"
};

static const char* proc_status_names[] = {
    [STOPPED] = "Stopped",
    [RUNNING] = "Running",
    [KILLED] = "Killed",
    [PAUSED] = "Paused",
    [EXITED] = "Exited"
};

// Example ring 3 user program
static unsigned char user_program[] = {
    // A simple infinite loop to test user mode execution.
    0xEB, 0xFE // jmp $
};

// Example ring 3 user program that calls the exit syscall
static unsigned char user_program_syscall_exit[] = {
    0xB8, 0x33, 0x00, 0x00, 0x00,  // mov eax, 51 (SYSCALL_PROC_EXIT)
    0xBB, 0xFF, 0xFF, 0xFF, 0xFF,  // mov ebx, -1
    0xCD, 0x80,                    // int 0x80
    0xEB, 0xFE                     // jmp $
};

static process_t* _proc_create_internal(bool restore_interrupts, proc_type_t kind, proc_entry_point_t kernel_entry, unsigned char* user_code, size_t user_size) {
    asm volatile("cli"); // Critical section

    // Guard
    if(!init_done) {
        LOG_ERR("PROC: proc_create called before proc_init!\n");
        if(restore_interrupts) {
            asm volatile("sti");
        }
        return NULL;
    }

    // Find slot
    process_t* proc = NULL;
    int proc_idx = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].used) {
            proc = &process_table[i];
            proc_idx = i;
            break;
        }
    }
    if (!proc) {
        LOG_ERR("PROC: No free process slots!\n");
        if(restore_interrupts) {
            asm volatile("sti");
        }
        return NULL;
    }
    LOG_DEBUG("PROC: Using slot %d\n", proc_idx);

    // Allocate a kernel stack
    proc->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    if (!proc->kernel_stack) {
        LOG_ERR("PROC: Failed to allocate kernel stack.\n");
        if(restore_interrupts) {
            asm volatile("sti");
        }
        return NULL;
    }
    proc->kernel_stack_size = KERNEL_STACK_SIZE;

    // Create page directories for user only if USER_PROC
    if (kind == USER_PROC) {
        proc->page_directory = vmm_create_user_directory();
        if (!proc->page_directory) {
            LOG_ERR("PROC: Failed to create user page directory.\n");
            kfree(proc->kernel_stack);
            proc->used = false;
            if(restore_interrupts) {
                asm volatile("sti");
            }
            return NULL;
        }
    } else {
        proc->page_directory = vmm_get_kernel_directory();
    }

    // Copy VFS descriptors from current process
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->open_files[i] = NULL;
    }
    process_t* parent = proc_get_current();
    memcpy(proc->open_files, parent->open_files, sizeof(parent->open_files));

    // Allocate and map user memory if needed
    kuint32_t user_stack_top = 0;
    if (kind == USER_PROC && user_code && user_size > 0) {
        physical_addr_t code_phys = (physical_addr_t)pmm_alloc_block();
        physical_addr_t stack_phys = (physical_addr_t)pmm_alloc_block();

        if (!code_phys || !stack_phys) {
            LOG_ERR("PROC: Failed to allocate user memory.\n");
            if (code_phys) pmm_free_block((generic_ptr)code_phys);
            if (stack_phys) pmm_free_block((generic_ptr)stack_phys);
            kfree(proc->kernel_stack);
            proc->used = false;
            if(restore_interrupts) {
                asm volatile("sti");
            }
            return NULL;
        }

        virtual_addr_t code_virt = 0x1000000;
        virtual_addr_t stack_virt = 0x1080000;
        user_stack_top = stack_virt + PMM_BLOCK_SIZE;
        user_stack_top &= ~0xF;

        // Map pages
        vmm_map_page_dir(proc->page_directory, code_virt, code_phys, PTE_PRESENT | PTE_USER);
        vmm_map_page_dir(proc->page_directory, stack_virt, stack_phys, PTE_PRESENT | PTE_USER | PTE_READ_WRITE);

        // Copy user code into memory
        memcpy((char*)code_phys, user_code, user_size);

        LOG_DEBUG("PROC: User code mapped at 0x%x, stack at 0x%x\n", code_virt, stack_virt);
    }

    // Setup initial stack frame
    char* kstack_ptr = (char*)proc->kernel_stack + KERNEL_STACK_SIZE;

    if (kind == USER_PROC) {
        // Allocate space for registers_t
        kstack_ptr -= sizeof(registers_t);
        registers_t* regs = (registers_t*)kstack_ptr;
        memset(regs, 0, sizeof(registers_t));

        // Set segment registers for user
        regs->ds = 0x23;
        regs->es = 0x23;
        regs->fs = 0x23;
        regs->gs = 0x23;

        // Push dummy interrupt/error for context_switch
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0; // int number
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0; // error code

        // Push IRET frame
        kuint32_t user_entry = 0x1000000;
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0x23;      // SS
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = user_stack_top; // ESP
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0x202;    // EFLAGS
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0x1B;     // CS
        kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = user_entry; // EIP

        proc->esp = (kuint32_t)kstack_ptr;

    } else {
        // Kernel process: ESP points directly at registers_t
        kstack_ptr -= sizeof(registers_t);
        registers_t* regs = (registers_t*)kstack_ptr;
        memset(regs, 0, sizeof(registers_t));

        regs->ds = 0x10;
        regs->es = 0x10;
        regs->fs = 0x10;
        regs->gs = 0x10;

        regs->eip = (kuint32_t)kernel_entry;
        regs->cs = 0x08;
        regs->eflags = 0x202; // interrupts enabled

        proc->esp = (kuint32_t)regs;
    }

    // Finalize process structure
    proc->process_id = next_pid++;
    proc->parent_proc_id = parent->process_id;
    proc->current_state = FIRST_RUN;
    proc->used = true;
    proc->proc_type = (kind == USER_PROC) ? USER_PROC : KERNEL_PROC;

    LOG_DEBUG("PROC: Created %s process PID %d\n",
              (kind == USER_PROC) ? "User" : "Kernel",
              proc->process_id);

    asm volatile("sti");
    return proc;
}

int proc_init() {
    LOG_DEBUG("PROC: Initializing process table...\n");
    for(int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].used = false;
        process_table[i].current_state = STOPPED;
    }

    // The kernel's main thread (PID 0) is the initial process and will become the idle task.
    process_table[0].used = true;
    process_table[0].current_state = FIRST_RUN;
    process_table[0].process_id = 0;
    process_table[0].esp = 0;
    process_table[0].proc_type = KERNEL_PROC;
    process_table[0].page_directory = vmm_get_kernel_directory();
    process_table[0].open_files[0] = NULL;                          // stdin
    process_table[0].open_files[1] = vfs_get_terminal_node();       // stdout
    process_table[0].open_files[2] = vfs_get_terminal_node();       // stderr
    process_table[0].open_files[3] = vfs_get_serial_com1_node();    // Serial COM1
    process_table[0].open_files[4] = vfs_get_serial_com1_node();    // Serial COM2
    current_process_index = 0;
    init_done = true;
    LOG_DEBUG("PROC: Initialization complete.\n");
    return 0;
}

process_t* proc_create(proc_entry_point_t entry_point, bool restore_interrupts) {
    LOG_DEBUG("-- Creating Kernel Process --\n");

    process_t* proc = _proc_create_internal(restore_interrupts, KERNEL_PROC, entry_point, NULL, 0);
    if (!proc) {
        LOG_ERR("Failed to create Kernel process.\n");
    } else {
        LOG_DEBUG("Kernel process PID %d created successfully!\n", proc->process_id);
    }
    return proc;
}

void create_user_process() {
    LOG_DEBUG("-- Creating User Process --\n");

    process_t* proc = _proc_create_internal(false, USER_PROC, NULL, user_program, sizeof(user_program));
    if (!proc) {
        LOG_ERR("Failed to create user process.\n");
    } else {
        LOG_DEBUG("User process PID %d created successfully!\n", proc->process_id);
    }
}

void create_user_process_syscall_exit() {
    LOG_DEBUG("-- Creating User Process --\n");

    process_t* proc = _proc_create_internal(false, USER_PROC, NULL, user_program_syscall_exit, sizeof(user_program_syscall_exit));
    if (!proc) {
        LOG_ERR("Failed to create user process.\n");
    } else {
        LOG_DEBUG("User process PID %d created successfully!\n", proc->process_id);
    }
}

process_t* proc_get_current() {
    if (!init_done) {
        return NULL;
    }
    return &process_table[current_process_index];
}

void proc_terminate(process_t* proc) {
    if(proc) {
        proc->current_state = EXITED;
        // In a more advanced kernel, we would free memory, close files, etc...
    }
}

void proc_scheduler_run(registers_t *regs) {
    if(!init_done) {
        return;
    }

    // Save the ESP of the current process. This points to the register struct.
    process_table[current_process_index].esp = (kuint32_t)regs;

    // Simple round-robin scheduler to find the next runnable process
    kuint32_t next_process_index = current_process_index;
    int attempts = 0;
    do {
        next_process_index = (next_process_index + 1) % MAX_PROCESSES;
        if (process_table[next_process_index].used && (process_table[next_process_index].current_state == RUNNING || process_table[next_process_index].current_state == FIRST_RUN)) {
            break; // Found a runnable process
        }
        attempts++;
    } while(attempts < MAX_PROCESSES);

    // If we looped through all processes and didn't find a runnable one,
    // just stay on the current process (which is likely the idle task).
    if (next_process_index == current_process_index) {
        // Before returning, we must restore the esp of the current process,
        // because the context_switch call expects it.
        // In this case, we are not switching, so we just return.
        return;
    }

    // We found a new process to switch to.
    current_process_index = next_process_index;
    process_t* next_proc = &process_table[next_process_index];

    // Update the TSS with the new process's kernel stack pointer.
    // This is crucial for handling interrupts that occur while in user mode.
    kuint32_t kernel_stack_top = (kuint32_t)next_proc->kernel_stack + next_proc->kernel_stack_size;
    tss_set_stack(0x10, kernel_stack_top); // 0x10 is our kernel data segment selector

    // Perform the context switch.
    // This will load the new process's ESP, pop all the registers off its
    // stack, and IRET to it. Control will not return here for this process.
    load_page_directory(next_proc->page_directory);
    if(next_proc->proc_type == USER_PROC && next_proc->current_state == FIRST_RUN) {
        next_proc->current_state = RUNNING;
        first_time_user_switch(next_proc->esp);
    } else {
        context_switch(next_proc->esp);
    }
}

