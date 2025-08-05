.intel_syntax noprefix
.global idt_load
.type idt_load, @function
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret
