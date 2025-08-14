#include <kernel/proc.h>
#include <kernel/heap.h>
#include <libc/strings.h>
#include <arch/i386/vmm.h>
#include <kernel/log.h>
#include <arch/i386/pmm.h>
#include <kernel/vfs.h>

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

    // --- Proc VFS ---
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        new_proc->open_files[i] = NULL;
    }

    file_node_t* terminal = vfs_get_terminal_node();
    new_proc->open_files[1] = terminal;
    new_proc->open_files[2] = terminal;

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
    process_t* next_proc = &process_table[next_process_index];

    // Update the current process index.
    current_process_index = next_process_index;

    // Update the TSS before we do a context switch
    kuint32_t kernel_stack_top = (kuint32_t)next_proc->kernel_stack + next_proc->kernel_stack_size;
    tss_set_stack(0x10, kernel_stack_top); // 0x10 is our kernel data segment selector

    // Perform the context switch.
    context_switch(next_proc->esp);
}

static unsigned char user_program[] = {
    0xB8, 0x3C, 0x00, 0x00, 0x00, // mov eax, 60 (SYSCALL_VFS_WRITE)
    0xBB, 0x01, 0x00, 0x00, 0x00, // mov ebx, 1  (fd stdout)
    0xB9, 0x22, 0x00, 0x10, 0x00, // mov ecx, 0x100022 (CORRECT address of the message)
    0xBA, 0x13, 0x00, 0x00, 0x00, // mov edx, 19 (message length)
    0xCD, 0x80,                   // int 0x80 (syscall)
    0xB8, 0x33, 0x00, 0x00, 0x00, // mov eax, 51 (SYSCALL_PROC_EXIT)
    0xBB, 0x00, 0x00, 0x00, 0x00, // mov ebx, 0  (exit status)
    0xCD, 0x80,                   // int 0x80 (syscall)
    'H', 'e', 'l', 'l', 'o', ' ', 'f', 'r', 'o', 'm', ' ', 'R', 'i', 'n', 'g', ' ', '3', '!', '\n'
};

void create_user_process() {
    // 1. Define the virtual memory layout for the new process.
    //    The user program expects to be at these specific virtual addresses.
    virtual_addr_t user_code_virt_addr = 0x100000;
    virtual_addr_t user_stack_virt_addr = 0x180000;
    virtual_addr_t user_stack_top = user_stack_virt_addr + PMM_BLOCK_SIZE; // Stack grows down

    // 2. Allocate physical memory for the code and stack using the PMM.
    physical_addr_t user_code_phys_addr = (physical_addr_t)pmm_alloc_block();
    physical_addr_t user_stack_phys_addr = (physical_addr_t)pmm_alloc_block();

    if (!user_code_phys_addr || !user_stack_phys_addr) {
        LOG_ERR("Failed to allocate physical memory for user process!\n");
        // Clean up any memory that was allocated before failing.
        if (user_code_phys_addr) pmm_free_block((generic_ptr)user_code_phys_addr);
        if (user_stack_phys_addr) pmm_free_block((generic_ptr)user_stack_phys_addr);
        return;
    }

    // 3. Create a new process structure.
    process_t* proc = proc_create(NULL, false);
    if (!proc) {
        LOG_ERR("Failed to create process structure!\n");
        // Clean up the memory we allocated.
        pmm_free_block((generic_ptr)user_code_phys_addr);
        pmm_free_block((generic_ptr)user_stack_phys_addr);
        return;
    }

    // 4. Map the physical pages into the process's virtual address space.
    // Code page: User-accessible, Read-only for user
    vmm_map_page(user_code_virt_addr, user_code_phys_addr, PTE_USER);

    // Stack page: User-accessible, Writable for user
    vmm_map_page(user_stack_virt_addr, user_stack_phys_addr, PTE_USER | PTE_READ_WRITE);

    // 5. Copy the user program to the newly allocated physical page.
    //    We can use the physical address directly because the PMM allocates from
    //    low memory, which is identity-mapped by the kernel's page directory.
    memcpy((char*)user_code_phys_addr, user_program, sizeof(user_program));

    // 6. Set up the interrupt frame to jump to user mode.
    registers_t* regs = (registers_t*)proc->esp;
    kuint32_t user_cs = 0x1B;
    kuint32_t user_ss = 0x23;

    regs->eip = user_code_virt_addr;
    regs->cs = user_cs;
    regs->eflags = 0x202; // Interrupts enabled
    regs->useresp = user_stack_top; // Set stack pointer to the top of the stack page
    regs->ss = user_ss;

    // Set data segments to the user data selector
    regs->ds = user_ss;
    regs->es = user_ss;
    regs->fs = user_ss;
    regs->gs = user_ss;
}
