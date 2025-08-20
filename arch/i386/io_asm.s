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

.global outl
outl:
    mov dx, [esp + 4]   # port
    mov eax, [esp + 8]  # value
    out dx, eax
    ret

.global inl
inl:
    mov dx, [esp + 4]   # port
    in eax, dx
    ret                 # return value is already in EAX

.global io_wait
io_wait:
    # Port 0x80 is often used for I/O delays
    mov al, 0x01
    out 0x80, al
    ret
