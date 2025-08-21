.global stack_top
.global boot_page_directory

# ----------------------------
# Multiboot header
# ----------------------------
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# ----------------------------
# Stack
# ----------------------------
.section .bootstrap_stack, "aw", @nobits
.global stack_top
stack_bottom:
    .skip 16384       # 16 KiB stack
stack_top:

# ----------------------------
# Page tables
# ----------------------------
.section .bss, "aw", @nobits
    .align 4096
boot_page_directory:
    .skip 4096
boot_page_table1:
    .skip 4096
    .align 4
saved_magic:
    .long 0
saved_mbi:
    .long 0

saved_magic_addr = 0x1000
saved_mbi_addr   = 0x1004

# ----------------------------
# Kernel entry point
# ----------------------------
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
    movl %eax, saved_magic_addr   # save magic
    movl %ebx, saved_mbi_addr     # save mbi pointer

    # ------------------------
    # Map first 4MB (identity) + higher half (0xC0000000)
    # ------------------------
    movl $0, %esi            # start physical addr
    movl $(boot_page_table1 - 0xC0000000), %edi
    movl $1023, %ecx         # 1023 entries = 4 MB

1:
    # Map all pages as present | writable
    movl %esi, %edx
    orl $0x003, %edx
    movl %edx, (%edi)
    addl $4096, %esi         # next physical page
    addl $4, %edi            # next PTE
    loop 1b

    # Map VGA text memory (0xB8000 -> 0xC03FF000)
    movl $(0x000B8000 | 0x003), boot_page_table1 - 0xC0000000 + 1023 * 4

    # Map page table to directory entries 0 (identity) + 768 (higher half)
    movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 0
    movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 768*4

    # Load page directory
    movl $boot_page_directory, %eax   # load symbol address into eax
    subl $0xC0000000, %eax            # subtract higher-half offset
    movl %eax, %cr3                    # load CR3

    # Enable paging + write-protect
    movl %cr0, %eax
    orl $0x80010000, %eax
    movl %eax, %cr0

    # ------------------------
    # Jump to higher half
    # ------------------------
    lea 4f, %eax
    jmp *%eax

.section .text
4:
    # ------------------------
    # Set up stack safely
    # ------------------------
    lea stack_top - 0x100, %esp       # safe stack margin

    # ------------------------
    # Call kernel_main(magic, mbi)
    # ------------------------
    push saved_mbi_addr
    push saved_magic_addr
    call kernel_main
    add $8, %esp

    # ------------------------
    # Halt forever
cli_halt:
    cli
    hlt
    jmp cli_halt
