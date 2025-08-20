.intel_syntax noprefix

.global load_page_directory
.global enable_paging
.global flush_tlb_single
.global read_cr2

# Loads the physical address of the page directory into CR3
load_page_directory:
    mov eax, [esp + 4]    # Get the address from the stack
    mov cr3, eax
    ret

# Enables the paging bit (PG) in the CR0 register
enable_paging:
    mov eax, cr0
    or eax, 0x80000000   # Set the PG bit (bit 31)
    mov cr0, eax
    ret

# Invalidates a single page in the TLB
flush_tlb_single:
    mov eax, [esp + 4]    # Get the virtual address
    invlpg [eax]
    ret

# Reads the CR2 register, which contains the faulting address on a page fault
read_cr2:
    mov eax, cr2
    ret