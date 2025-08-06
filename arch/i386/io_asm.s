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

.global io_wait
io_wait:
    # Port 0x80 is often used for I/O delays, as it's a "checkpoint" port
    # used by the BIOS during POST. Writing to it causes a short delay.
    mov al, 0x01
    out 0x80, al
    ret
