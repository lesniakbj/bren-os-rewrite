.intel_syntax noprefix
.global gdt_load
.type gdt_load, @function
gdt_load:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:flush_gdt

flush_gdt:
    ret
