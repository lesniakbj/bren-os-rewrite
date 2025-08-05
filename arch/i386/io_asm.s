.intel_syntax noprefix

.global outb
outb:
    mov dx, [esp + 4]   # port
    mov al, [esp + 8]   # value
    out dx, al
    ret

.global inb
inb:
    mov dx, [esp + 4]   # port
    in al, dx
    movzx eax, al       # return value in EAX
    ret
