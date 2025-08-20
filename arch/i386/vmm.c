#include <arch/i386/vmm.h>
#include <arch/i386/pmm.h>
#include <kernel/log.h>

// Pointers to our page directory and a page table
pde_t* page_directory = 0;
pte_t* first_page_table = 0;

// Assembly functions defined in vmm_asm.s
void load_page_directory(pde_t* page_directory_physical_addr);
void enable_paging();
void flush_tlb_single(kuint32_t virtual_addr);
kuint32_t read_cr2();

vmm_init_status_t vmm_init(multiboot_info_t* mbi) {
    LOG_DEBUG("Setting up VMM...");

    // Allocate frames for the page directory and one page table
    page_directory = (pde_t*)pmm_alloc_block();
    first_page_table = (pte_t*)pmm_alloc_block();

    if (!page_directory || !first_page_table) {
        LOG_ERR("VMM Error: Failed to allocate frames for paging structures.");
        return;
    }

    // Clear the allocated memory to ensure no stale data
    memset(page_directory, 0, PAGE_SIZE);
    memset(first_page_table, 0, PAGE_SIZE);

    // Identity map the first 4MB of memory in our first page table
    for (int i = 0; i < TABLE_ENTRIES; i++) {
        kuint32_t frame_address = i * PAGE_SIZE;
        first_page_table[i] = frame_address | PTE_PRESENT | PTE_READ_WRITE;
    }

    // Point the first entry of the page directory to our page table.
    // The address must be the PHYSICAL address of the page table.
    page_directory[0] = (pde_t)first_page_table | PDE_PRESENT | PDE_READ_WRITE;

    // Identity map the framebuffer memory region if available
    if (mbi && CHECK_MULTIBOOT_FLAG(mbi->flags, 12) && mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        physical_addr_t framebuffer_start = (physical_addr_t)mbi->framebuffer_addr;
        physical_addr_t framebuffer_size = mbi->framebuffer_height * mbi->framebuffer_pitch;
        physical_addr_t framebuffer_end = framebuffer_start + framebuffer_size;
        
        LOG_DEBUG("Mapping framebuffer: 0x%x - 0x%x", framebuffer_start, framebuffer_end);
        
        // Map each 4KB page of the framebuffer
        for (physical_addr_t addr = framebuffer_start; addr < framebuffer_end; addr += PAGE_SIZE) {
            vmm_identity_map_page(addr);
        }
    }

    // Load the physical address of the page directory into the CR3 register
    load_page_directory(page_directory);

    // Enable paging by setting the PG bit in the CR0 register
    enable_paging();

    LOG_DEBUG("Paging enabled.");
}

void vmm_identity_map_page(physical_addr_t physical_addr) {
    // Decompose the physical address into a Directory and Table index, address must be aligned first
    physical_addr_t page_addr = physical_addr & 0xFFFFF000;   // Align to 4KB boundary
    kuint32_t pde_index = page_addr >> 22;
    kuint32_t pte_index = (page_addr >> 12) & 0x3FF;

    pde_t* pde = &page_directory[pde_index];
    pte_t* page_table;

    // If the page table for this address range isn't present, we need to create one.
    // This involves allocating a new physical page to serve as the page table,
    // clearing it, and then updating the Page Directory Entry (PDE) to hold
    // the physical address of this new page table.
    if ((*pde & PDE_PRESENT) == 0) {
        page_table = (pte_t*)pmm_alloc_block();
        if (!page_table) {
            LOG_DEBUG("VMM Error: Out of memory creating new page table!");
            return;
        }
        memset(page_table, 0, PAGE_SIZE);
        *pde = (pde_t)page_table | PDE_PRESENT | PDE_READ_WRITE;
    }

    // The page directory contains physical addresses. To access the page table,
    // we need a virtual address. Since PMM allocates from low memory (which is
    // identity-mapped), we can directly cast the physical address to a virtual one.
    // This assumption must be revisited for a more advanced VMM.
    page_table = (pte_t*)(*pde & PDE_FRAME);

    // Map the page if it's not already mapped
    if ((page_table[pte_index] & PTE_PRESENT) == 0) {
        page_table[pte_index] = page_addr | PTE_PRESENT | PTE_READ_WRITE;

        // Invalidate the TLB entry for the modified page.
        flush_tlb_single(page_addr);
    }
}

void vmm_map_page(virtual_addr_t virtual_addr, physical_addr_t physical_addr, kuint32_t flags) {
    // Decompose the physical address into a Directory and Table index, address must be aligned first
    virtual_addr_t aligned_addr = (virtual_addr & 0xFFFFF000);
    kuint32_t page_directory_idx = aligned_addr >> 22;
    kuint32_t page_table_idx = (aligned_addr >> 12) & 0x3FF;
    
    // Get a pointer to the page directory entry for this address
    pde_t* page_dir = &page_directory[page_directory_idx];
    pte_t* page_table;

    // If the page table for this address range isn't present, we need to create one.
    // This involves allocating a new physical page to serve as the page table,
    // clearing it, and then updating the Page Directory Entry (PDE) to hold
    // the physical address of this new page table.
    if (!(*page_dir & PDE_PRESENT)) {
        page_table = (pte_t*)pmm_alloc_block();
        if(!page_table) {
            LOG_ERR("VMM Error: Out of memory creating new page table!");
            return;
        }
        memset(page_table, 0, PAGE_SIZE);
        *page_dir = (pde_t) page_table | PDE_PRESENT | PDE_READ_WRITE;
    } else {
        // Get the virtual address of the page table
        page_table = (pte_t*)(*page_dir & PDE_FRAME);
    }

    // Create the page table entry (PTE) with the physical address and flags
    pte_t table_entry = physical_addr | flags | PTE_PRESENT;
    
    // Set the page table entry
    page_table[page_table_idx] = table_entry;
    
    // Invalidate the TLB entry for the virtual address
    flush_tlb_single(aligned_addr);
}

void vmm_unmap_page(virtual_addr_t virtual_addr) {
    // Decompose the physical address into a Directory and Table index, address must be aligned first
    virtual_addr_t aligned_addr = (virtual_addr & 0xFFFFF000);
    kuint32_t page_directory_idx = aligned_addr >> 22;
    kuint32_t page_table_idx = (aligned_addr >> 12) & 0x3FF;


    pde_t* page_dir = &page_directory[page_directory_idx];

    // Check if the page table is present
    if(!(*page_dir & PDE_PRESENT)) {
        LOG_ERR("VMM Error: No page found to free, returning.");
        return;
    }

    // Get the virtual address of the page table so we can clear its entry
    pte_t* page_table = (pte_t*)(*page_dir & PDE_FRAME);
    page_table[page_table_idx] = 0;
    
    // Invalidate the TLB entry for the virtual address
    flush_tlb_single(aligned_addr);
}

physical_addr_t vmm_get_physical_addr(virtual_addr_t virtual_addr) {
    // Save the offset within the page (lower 12 bits), align the address and get the indexes into PDE/PTE
    kuint32_t offset = virtual_addr & 0xFFF;
    kuint32_t aligned_addr = virtual_addr & 0xFFFFF000;
    kuint32_t page_directory_idx = aligned_addr >> 22;
    kuint32_t page_table_idx = (aligned_addr >> 12) & 0x3FF;


    // Check if the page table is present
    pde_t* page_dir = &page_directory[page_directory_idx];
    if(!(*page_dir & PDE_PRESENT)) {
        LOG_ERR("VMM Error: Invalid address!");
        return 0;
    }
    
    // Get the virtual address of the page table and if its present
    pte_t* page_table = (pte_t*) (*page_dir & PDE_FRAME);
    if(!(page_table[page_table_idx] & PTE_PRESENT)) {
        LOG_ERR("VMM Error: Invalid address!");
        return 0;
    }

    // Get the physical page address
    physical_addr_t addr = page_table[page_table_idx] & PTE_FRAME;

    // Combine the physical page address with the saved offset
    return addr | offset;
}

// Creates a new user page directory, identity-maps the kernel
pde_t* vmm_create_user_directory() {
    // Allocate one page for the new page directory
    pde_t* new_pd = (pde_t*)pmm_alloc_block();
    if (!new_pd) {
        LOG_ERR("VMM: Failed to allocate page directory!");
        return NULL;
    }
    memset(new_pd, 0, PAGE_SIZE);

    // Copy the kernel mappings from the global page_directory (higher-half)
    for (int i = 768; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    return new_pd;
}

pde_t* vmm_get_kernel_directory() {
    return page_directory;
}

void page_fault_handler(registers_t *regs){
    kuint32_t faulting_address = read_cr2();

    // The error code gives us details about the fault.
    int present = !(regs->error_code & 0x1);
    int rw = regs->error_code & 0x2;
    int us = regs->error_code & 0x4;
    int reserved = regs->error_code & 0x8;
    int id = regs->error_code & 0x10;

    LOG_ERR("--- PAGE FAULT ---");
    LOG_ERR("Faulting Address: 0x%x", faulting_address);
    LOG_ERR("Reason: %s, %s, %s, %s, %s",
        present ? "Present" : "Not Present",
        rw ? "Write" : "Read",
        us ? "User-mode" : "Kernel-mode",
        reserved ? "Reserved Bit Set" : "",
        id ? "Instruction Fetch" : "");
    LOG_ERR("EIP: 0x%x", regs->eip);
    LOG_ERR("System Halted.");
    for(;;);
}

