#include <kernel/log.h>
#include <kernel/proc.h>
#include <arch/i386/fault.h>

void general_protection_fault_handler(registers_t* regs) {
    process_t* current_proc = proc_get_current();
    
    LOG_ERR("--- GENERAL PROTECTION FAULT ---");
    LOG_ERR("Fault occurred in process PID %d", current_proc ? current_proc->process_id : -1);
    LOG_ERR("Segments and IP:");
    LOG_ERR("\tCS:EIP: 0x%x:0x%x", regs->cs, regs->eip);
    LOG_ERR("\tDS: 0x%x", regs->ds);
    LOG_ERR("\tES: 0x%x", regs->es);
    LOG_ERR("\tFS: 0x%x", regs->fs);
    LOG_ERR("\tGS: 0x%x", regs->gs);
    LOG_ERR("Registers:");
    LOG_ERR("\tEDI: 0x%x", regs->edi);
    LOG_ERR("\tESI: 0x%x", regs->esi);
    LOG_ERR("\tEBP: 0x%x", regs->ebp);
    LOG_ERR("\tESP: 0x%x", regs->esp);
    LOG_ERR("\tEBX: 0x%x", regs->ebx);
    LOG_ERR("\tEDX: 0x%x", regs->edx);
    LOG_ERR("\tECX: 0x%x", regs->ecx);
    LOG_ERR("\tEAX: 0x%x", regs->eax);
    LOG_ERR("Flags and Stacks:");
    LOG_ERR("\tEFLAGS: 0x%x", regs->eflags);
    LOG_ERR("\tUSERESP: 0x%x", regs->useresp);
    LOG_ERR("\tSS: 0x%x", regs->ss);
    if(regs->error_code) {
        LOG_ERR("SEGMENT ERR: 0x%x", regs->error_code); // ss idx
    }
    
    // If this is a user process, terminate it instead of crashing the system
    if (current_proc && current_proc->proc_type == USER_PROC) {
        LOG_ERR("Terminating user process PID %d due to privilege violation", current_proc->process_id);
        proc_terminate(current_proc);
        
        // Force a context switch to another process
        proc_scheduler_run(regs);
        
        // This point should never be reached
        LOG_ERR("System Halted after failed process termination.");
    } else {
        LOG_ERR("System Halted.");
    }
    
    for(;;);
}