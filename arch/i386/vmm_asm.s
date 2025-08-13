.global load_page_directory
.global enable_paging
.global flush_tlb_single
.global read_cr2

# Loads the physical address of the page directory into CR3
# C-callable function: void load_page_directory(pde_t* page_directory_physical_addr);
load_page_directory:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax    # Get the address from the stack
    movl %eax, %cr3
    popl %ebp
    ret

# Enables the paging bit (PG) in the CR0 register
# C-callable function: void enable_paging();
enable_paging:
    movl %cr0, %eax
    orl $0x80000000, %eax   # Set the PG bit (bit 31)
    movl %eax, %cr0
    ret

# Invalidates a single page in the TLB
# C-callable function: void flush_tlb_single(kuint32_t virtual_addr);
flush_tlb_single:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax    # Get the virtual address
    invlpg (%eax)
    popl %ebp
    ret

# Reads the CR2 register, which contains the faulting address on a page fault
# C-callable function: kuint32_t read_cr2();
read_cr2:
    movl %cr2, %eax
    ret

