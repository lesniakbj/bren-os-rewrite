# OS-Dev Project

## Project Overview

This project is an experimental operating system developed from scratch. It aims to provide a foundational understanding of how operating systems work, from bootloading to basic kernel functionalities. This OS is designed for educational purposes and serves as a learning platform for low-level system programming.

<!-- Images of the OS will go here -->

## Features

*   **Bootloader:** Uses GNU GRUB to boot the kernel.
*   **Kernel:** A 32-bit kernel written in C.
*   **Memory Management:** Basic physical memory management.
*   **Interrupts:** Handles basic hardware interrupts.
*   **Drivers:** Includes drivers for the keyboard, mouse, and screen.
*   **Real-Time Clock (RTC):** Reads the current time from the CMOS.

## Getting Started

To get this project up and running, you'll need a few prerequisites and then follow the build and run steps.

### Prerequisites

*   **GCC Cross-Compiler for i686-elf:** This project requires a cross-compiler targeting `i686-elf`. You can build one using the provided `.toolchain/build-cross.sh` script.
*   **GNU GRUB:** GRUB is used as the bootloader.
*   **QEMU:** QEMU is used for emulating the hardware to run the OS.
*   **Make:** For building the project.

### Environment Setup (Downloads dependencies, builds the cross compiler, ...)

```bash
./env-setup.sh
```

### Building the OS

```bash
make clean && make
```

### Running the OS in QEMU

```bash
make run
```

## For Developers

Welcome, fellow OS enthusiast! This section provides some notes for those looking to dive into the codebase.

*   **Boot Process:** Start by examining `arch/i386/boot.s` for the initial boot sequence and `kernel/kernel.c` for the kernel entry point.
*   **Interrupts and GDT/IDT:** The `arch/i386/` directory contains the core assembly and C files for setting up the Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), and interrupt handlers.
*   **Drivers:** Basic drivers for screen output (`drivers/screen.c`, `drivers/terminal.c`, `drivers/framebuffer_console.c`, `drivers/text_mode_console.c`) and keyboard input (`drivers/keyboard.c`) are located in the `drivers/` directory.
*   **Memory Management:** (Future work/area for expansion) Currently, memory management is very basic. This is a key area for future development.
*   **Toolchain:** The `.toolchain/` directory contains scripts and source code for building the necessary cross-compiler and debugging tools. Ensure your `PATH` includes the cross-compiler's `bin` directory.
*   **Debugging:** For debugging, ensure QEMU is running in debug mode (e.g., `make debug`). Then, use the `i686-elf-gdb` from your toolchain. You can load the provided `gdb.script` to set up common breakpoints and symbols. For proper source-level debugging, ensure you load the kernel binary with debugging symbols.

    Example GDB command:
    ```bash
    ./.toolchain/cross/bin/i686-elf-gdb -x gdb.script .toolchain/iso/boot/test-os.bin
    ```
    
    Once in GDB, connect to QEMU:
    ```
    (gdb) target remote :1234
    ```
    (Assuming QEMU is listening on port 1234, which `make debug` typically aconfigures.)