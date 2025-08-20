.intel_syntax noprefix
.global gdt_load
.type gdt_load, @function
gdt_load:
    mov eax, [esp + 4]  # Point to and load the GDT (accesses first argument in the called C function)
    lgdt [eax]

    mov ax, 0x10        # Set segment selector to the kernel data segment (GDT index 2 -> 2 * 8 = 16 = 0x10).
    mov ds, ax          # Reload all segment registers with the data selector. Clears old segments.
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:flush_gdt  # Far jump to reload the CS register (0x08 for idx 1 in the GDT)

flush_gdt:
    ret

.global tss_flush
.type tss_flush, @function
tss_flush:
    mov ax, 0x2B | 0    # TSS GDT selector (5 * 8) with RPL=3
    ltr ax
    ret