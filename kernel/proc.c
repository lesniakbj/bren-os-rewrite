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

// The user program to execute in Ring 3
static unsigned char user_program[] = {
    // A simple infinite loop to test user mode execution.
    // If the kernel no longer crashes, the problem is with syscall handling.
    0xEB, 0xFE // jmp $
};


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
    process_table[0].esp = 0;
    process_table[0].open_files[0] = NULL;                      // stdin
    process_table[0].open_files[1] = vfs_get_terminal_node();   // stdout
    process_table[0].open_files[2] = vfs_get_terminal_node();   // stderr
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
            asm volatile("sti");
        }
        return NULL;
    }

    // Find a slot in the process table for this new process
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
            asm volatile("sti");
        }
        return NULL;
    }

    LOG_DEBUG("PROC: Creating new process in slot %d.\n", new_proc_idx);

    // Create the VFS table, inherit any open_files from the parent
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        new_proc->open_files[i] = NULL;
    }
    process_t* parent = proc_get_current();
    LOG_DEBUG("PROC: Copying VFS descriptors from parent %d\n", parent->process_id);
    memcpy(new_proc->open_files, parent->open_files, sizeof(parent->open_files));

    // Allocate a kernel stack
    new_proc->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    if (new_proc->kernel_stack == NULL) {
        LOG_ERR("PROC: Failed to allocate kernel stack for new process.\n");
        if(restore_interrupts) {
            asm volatile("sti");
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
    regs->cs = 0x08;        // Kernel code segment
    regs->eflags = 0x202;   // Interrupts enabled
    regs->gs = 0x10;        // Kernel data segment
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
        asm volatile("sti");
    }
    return new_proc;
}

void create_user_process() {
    asm volatile("cli"); // Enter critical section
    LOG_DEBUG("-- Creating User Process --\n");

    if(!init_done) {
        LOG_ERR("PROC: proc_create called before proc_init!\n");
        asm volatile("sti"); // Re-enable interrupts before returning
        return;
    }

    // 1. Find a free process slot
    process_t* proc = NULL;
    int proc_idx = -1;
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].used == false) {
            proc = &process_table[i];
            proc_idx = i;
            break;
        }
    }
    if (!proc) {
        LOG_ERR("Failed to find free process slot for user process!\n");
        asm volatile("sti");
        return;
    }
    LOG_DEBUG("Found free process slot: %d\n", proc_idx);

    // 2. Define virtual memory layout
    virtual_addr_t user_code_virt_addr = 0x100000;
    virtual_addr_t user_stack_virt_addr = 0x180000;
    virtual_addr_t user_stack_top = user_stack_virt_addr + PMM_BLOCK_SIZE;
    LOG_DEBUG("User Code VirtAddr: 0x%x, User Stack VirtAddr: 0x%x\n", user_code_virt_addr, user_stack_virt_addr);

    // 3. Allocate physical memory
    physical_addr_t user_code_phys_addr = (physical_addr_t)pmm_alloc_block();
    physical_addr_t user_stack_phys_addr = (physical_addr_t)pmm_alloc_block();
    proc->kernel_stack = kmalloc(KERNEL_STACK_SIZE);

    if (!user_code_phys_addr || !user_stack_phys_addr || !proc->kernel_stack) {
        LOG_ERR("Failed to allocate memory for user process!\n");
        if (user_code_phys_addr) pmm_free_block((generic_ptr)user_code_phys_addr);
        if (user_stack_phys_addr) pmm_free_block((generic_ptr)user_stack_phys_addr);
        if (proc->kernel_stack) kfree(proc->kernel_stack);
        proc->used = false;
        asm volatile("sti");
        return;
    }
    LOG_DEBUG("User Code PhysAddr: 0x%x, User Stack PhysAddr: 0x%x\n", user_code_phys_addr, user_stack_phys_addr);
    LOG_DEBUG("Kernel Stack Addr:  0x%x\n", (kuint32_t)proc->kernel_stack);

    // 4. Set up the process structure
    proc->process_id = next_pid++;
    proc->current_state = RUNNING;
    proc->used = true;
    proc->page_directory = vmm_get_kernel_directory();
    proc->kernel_stack_size = KERNEL_STACK_SIZE;
    LOG_DEBUG("Process %d assigned. Page dir: 0x%x\n", proc->process_id, (kuint32_t)proc->page_directory);

    // Create the VFS table, inherit any open_files from the parent
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->open_files[i] = NULL;
    }
    process_t* parent = proc_get_current();
    memcpy(proc->open_files, parent->open_files, sizeof(parent->open_files));

    // 6. Map pages into the address space
    LOG_DEBUG("Mapping 0x%x -> 0x%x (USER)\n", user_code_virt_addr, user_code_phys_addr);
    vmm_map_page(user_code_virt_addr, user_code_phys_addr, PTE_USER);
    LOG_DEBUG("Mapping 0x%x -> 0x%x (USER|WRITE)\n", user_stack_virt_addr, user_stack_phys_addr);
    vmm_map_page(user_stack_virt_addr, user_stack_phys_addr, PTE_USER | PTE_READ_WRITE);

    // 7. Copy the program into its physical page
    memcpy((char*)user_code_phys_addr, user_program, sizeof(user_program));
    LOG_DEBUG("Copied %d bytes of user program to 0x%x\n", sizeof(user_program), user_code_phys_addr);

    // 8. Set up the stack to match what context_switch expects.
    // This fabricates a stack frame that looks as if the process was
    // interrupted right at its entry point.
    char* kstack_ptr = (char*)proc->kernel_stack + KERNEL_STACK_SIZE;

    // The stack needs to be set up to look exactly like the stack
    // after an interrupt has occurred. The order of pushes is:
    // CPU -> SS, ESP, EFLAGS, CS, EIP
    // ISR Stub -> error code, interrupt number
    // isr_common_stub -> gs, fs, es, ds, then pusha
    // We build this structure from the bottom-up (pushing onto the stack).

    kuint32_t user_cs = 0x1B; // User code segment with RPL=3
    kuint32_t user_ss = 0x23; // User data segment with RPL=3

    // IRET frame
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = user_ss;
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = user_stack_top;
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0x202; // EFLAGS (IF = 1)
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = user_cs;
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = user_code_virt_addr;

    // Dummy interrupt number and error code
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0; // error code
    kstack_ptr -= sizeof(kuint32_t); *((kuint32_t*)kstack_ptr) = 0; // int number

    // General purpose registers (pushed by pusha) and segment registers
    // We use the registers_t struct which should match this layout.
    kstack_ptr -= sizeof(registers_t);
    registers_t* regs = (registers_t*)kstack_ptr;
    memset(regs, 0, sizeof(registers_t));

    // Set the user-mode data segments
    regs->ds = user_ss;
    regs->es = user_ss;
    regs->fs = user_ss;
    regs->gs = user_ss;

    // The process's ESP should point to the top of this entire structure.
    proc->esp = (kuint32_t)kstack_ptr;

    LOG_DEBUG("User process stack prepared for context_switch.\n");
    LOG_DEBUG("Process ESP set to: 0x%x\n", proc->esp);

    LOG_DEBUG("-- User Process Creation Complete --\n");
    asm volatile("sti"); // Exit critical section
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
        if (process_table[next_process_index].used && process_table[next_process_index].current_state == RUNNING) {
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
    context_switch(next_proc->esp);
}

