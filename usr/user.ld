/*
 * Simple linker script for user programs.
 */

ENTRY(main)

SECTIONS
{
    . = 0x08048000; /* Standard user-space starting address */

    .text :
    {
        *(.text)
        *(.rodata*)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }
}
