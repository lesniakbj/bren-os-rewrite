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
    LOG_DEBUG("Setting up VMM...\n");

    // Allocate frames for the page directory and one page table
    // These functions must return the physical address of the allocated frames.
    page_directory = (pde_t*)pmm_alloc_block();
    first_page_table = (pte_t*)pmm_alloc_block();

    if (!page_directory || !first_page_table) {
        LOG_ERR("VMM Error: Failed to allocate frames for paging structures.\n");
        // This is a fatal error, normally we would halt or panic.
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
        
        LOG_DEBUG("Mapping framebuffer: 0x%x - 0x%x\n", framebuffer_start, framebuffer_end);
        
        // Map each 4KB page of the framebuffer
        for (physical_addr_t addr = framebuffer_start; addr < framebuffer_end; addr += PAGE_SIZE) {
            vmm_identity_map_page(addr);
        }
    }

    // Load the physical address of the page directory into the CR3 register
    load_page_directory(page_directory);

    // Enable paging by setting the PG bit in the CR0 register
    enable_paging();

    LOG_DEBUG("Paging enabled.\n");
}

void vmm_identity_map_page(kuint32_t physical_addr) {
    kuint32_t page_addr = physical_addr & 0xFFFFF000; // Align to 4KB boundary
    kuint32_t pde_index = page_addr >> 22;
    kuint32_t pte_index = (page_addr >> 12) & 0x3FF;

    pde_t* pde = &page_directory[pde_index];
    pte_t* page_table;

    if ((*pde & PDE_PRESENT) == 0) {
        // Page table not present, we need to create one
        page_table = (pte_t*)pmm_alloc_block();
        if (!page_table) {
            LOG_DEBUG("VMM Error: Out of memory creating new page table!\n");
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

// --- STUBBED FUNCTIONS ---

void vmm_map_page(virtual_addr_t virtual_addr, physical_addr_t physical_addr, kuint32_t flags) {
    // STEP 1: Align the virtual address to a page boundary (4KB)
    //         Use bitwise AND with 0xFFFFF000 to clear the lower 12 bits
    virtual_addr_t aligned_addr = (virtual_addr & 0xFFFFF000);

    // STEP 2: Calculate the page directory index (upper 10 bits)
    //         Right shift the aligned address by 22 bits
    kuint32_t page_directory_idx = aligned_addr >> 22;

    // STEP 3: Calculate the page table index (middle 10 bits)
    //         Right shift the aligned address by 12 bits, then mask with 0x3FF
    kuint32_t page_table_idx = (aligned_addr >> 12) & 0x3FF;
    
    // STEP 4: Get a pointer to the page directory entry (PDE)
    //         Use the page directory index to access the correct entry in page_directory array
    pde_t* page_dir = &page_directory[page_directory_idx];

    // STEP 5: Declare a pointer for the page table
    pte_t* page_table;

    // STEP 6: Check if the page table is present
    //         Check if the PDE has the PDE_PRESENT bit set
    if (!(*page_dir & PDE_PRESENT)) {
        // STEP 7: If page table is not present:
        //         a. Allocate a new page for the page table using pmm_alloc_block()
        //         b. Check if allocation was successful (not NULL)
        //         c. If allocation failed, print error message and return
        //         d. Clear the newly allocated page table with memset()
        //         e. Set the PDE to point to the new page table with proper flags (PDE_PRESENT, PDE_READ_WRITE)
        page_table = (pte_t*)pmm_alloc_block();
        if(!page_table) {
            LOG_ERR("VMM Error: Out of memory creating new page table!\n");
            return;
        }
        memset(page_table, 0, PAGE_SIZE);
        *page_dir = (pde_t) page_table | PDE_PRESENT | PDE_READ_WRITE;
    } else {
        // STEP 8: Get the virtual address of the page table
        //         Extract the physical address from the PDE using PDE_FRAME mask
        //         Cast to pte_t* (this works because low memory is identity mapped)
        page_table = (pte_t*)(*page_dir & PDE_FRAME);
    }

    // STEP 9: Create the page table entry (PTE) with the physical address and flags
    //         Combine the physical address with the flags parameter and PTE_PRESENT
    pte_t table_entry = physical_addr | flags | PTE_PRESENT;
    
    // STEP 10: Set the page table entry
    //          Use the page table index to set the correct entry in the page table
    page_table[page_table_idx] = table_entry;
    
    // STEP 11: Invalidate the TLB entry for the virtual address
    //          Call flush_tlb_single() with the aligned virtual address
    flush_tlb_single(aligned_addr);
}

void vmm_unmap_page(virtual_addr_t virtual_addr) {
    // STEP 1: Align the virtual address to a page boundary (4KB)
    //         Use bitwise AND with 0xFFFFF000 to clear the lower 12 bits
    virtual_addr_t aligned_addr = virtual_addr & 0xFFFFF000;

    // STEP 2: Calculate the page directory index (upper 10 bits)
    //         Right shift the aligned address by 22 bits
    kuint32_t page_directory_idx = aligned_addr >> 22;

    // STEP 3: Calculate the page table index (middle 10 bits)
    //         Right shift the aligned address by 12 bits, then mask with 0x3FF
    kuint32_t page_table_idx = (aligned_addr >> 12) & 0x3FF;
    
    // STEP 4: Get a pointer to the page directory entry (PDE)
    //         Use the page directory index to access the correct entry in page_directory array
    pde_t* page_dir = &page_directory[page_directory_idx];

    // STEP 5: Check if the page table is present
    //         Check if the PDE has the PDE_PRESENT bit set
    //         If not present, return immediately (nothing to unmap)
    if(!(*page_dir & PDE_PRESENT)) {
        LOG_ERR("VMM Error: No page found to free, returning.\n");
        return;
    }

    // STEP 6: Get the virtual address of the page table
    //         Extract the physical address from the PDE using PDE_FRAME mask
    //         Cast to pte_t* (this works because low memory is identity mapped)
    pte_t* page_table = (pte_t*)(*page_dir & PDE_FRAME);

    // STEP 7: Clear the page table entry
    //         Set the page table entry at the calculated index to 0
    page_table[page_table_idx] = 0;
    
    // STEP 8: Invalidate the TLB entry for the virtual address
    //         Call flush_tlb_single() with the aligned virtual address
    flush_tlb_single(aligned_addr);
}

physical_addr_t vmm_get_physical_addr(virtual_addr_t virtual_addr) {
    // STEP 1: Save the offset within the page (lower 12 bits)
    //         Use bitwise AND with 0xFFF to extract the offset
    kuint32_t offset = virtual_addr & 0xFFF;

    // STEP 2: Align the virtual address to a page boundary (4KB)
    //         Use bitwise AND with 0xFFFFF000 to clear the lower 12 bits
    kuint32_t aligned_addr = virtual_addr & 0xFFFFF000;

    // STEP 3: Calculate the page directory index (upper 10 bits)
    //         Right shift the aligned address by 22 bits
    kuint32_t page_directory_idx = aligned_addr >> 22;

    // STEP 4: Calculate the page table index (middle 10 bits)
    //         Right shift the aligned address by 12 bits, then mask with 0x3FF
    kuint32_t page_table_idx = (aligned_addr >> 12) & 0x3FF;

    // STEP 5: Get a pointer to the page directory entry (PDE)
    //         Use the page directory index to access the correct entry in page_directory array
    pde_t* page_dir = &page_directory[page_directory_idx];

    // STEP 6: Check if the page table is present
    //         Check if the PDE has the PDE_PRESENT bit set
    //         If not present, return 0 (invalid address)
    if(!(*page_dir & PDE_PRESENT)) {
        LOG_ERR("VMM Error: Invalid address!\n");
        return 0;
    }
    
    // STEP 7: Get the virtual address of the page table
    //         Extract the physical address from the PDE using PDE_FRAME mask
    //         Cast to pte_t* (this works because low memory is identity mapped)
    pte_t* page_table = (pte_t*) (*page_dir & PDE_FRAME);

    // STEP 8: Check if the page is present
    //         Check if the PTE has the PTE_PRESENT bit set
    //         If not present, return 0 (invalid address)
    if(!(page_table[page_table_idx] & PTE_PRESENT)) {
        LOG_ERR("VMM Error: Invalid address!\n");
        return 0;
    }

    // STEP 9: Get the physical page address
    //         Extract the physical address from the PTE using PTE_FRAME mask
    physical_addr_t addr = page_table[page_table_idx] & PTE_FRAME;

    // STEP 10: Combine the physical page address with the saved offset
    //          Return the combination of physical page address and offset
    return addr | offset;
}

void page_fault_handler(struct registers *regs){
    kuint32_t faulting_address = read_cr2();

    // The error code gives us details about the fault.
    int present = !(regs->error_code & 0x1);
    int rw = regs->error_code & 0x2;
    int us = regs->error_code & 0x4;
    int reserved = regs->error_code & 0x8;
    int id = regs->error_code & 0x10;

    LOG_ERR("--- PAGE FAULT ---\n");
    LOG_ERR("Faulting Address: 0x%x\n", faulting_address);
    LOG_ERR("Reason: %s, %s, %s, %s, %s\n",
        present ? "Present" : "Not Present",
        rw ? "Write" : "Read",
        us ? "User-mode" : "Kernel-mode",
        reserved ? "Reserved Bit Set" : "",
        id ? "Instruction Fetch" : "");
    LOG_ERR("EIP: 0x%x\n", regs->eip);

    // Halt the system. In a real OS, we might try to recover or kill the process.
    LOG_ERR("System Halted.\n");
    for(;;);
}

