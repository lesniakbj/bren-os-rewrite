# Plan: Transition to EFI Bootloader

This document outlines the steps needed to transition your kernel from using GRUB (Multiboot) to using an EFI bootloader.

## Current State Analysis

Your current kernel uses:
- i686 architecture (32-bit)
- Multiboot specification for booting with GRUB
- Manual GDT setup in the kernel
- Multiboot information structure for system information
- Custom linker script placing kernel at 1MB
- Lower-half kernel (planning to move to higher-half)

## Transition Goals

1. Move to x86_64 architecture
2. Implement UEFI application entry point
3. Replace GRUB/Multiboot with EFI services
4. Simplify kernel entry and initialization
5. Combine with higher-half kernel implementation for a modern 64-bit kernel

## Phase 1: Toolchain and Architecture Changes

### 1.1 Update Cross-Compilation Toolchain
- Change target from `i686-elf` to `x86_64-elf`
- Update GCC, binutils, and GDB configurations
- Modify `env-setup.sh` to download x86_64 toolchain
- Update paths in all build scripts to use x86_64 toolchain

### 1.2 Update Makefile
- Change `CC` and `AS` variables to x86_64 versions
- Update compiler flags for 64-bit compilation (`-m64`, etc.)
- Add new targets for EFI binary generation
- Update all source file paths from `arch/i386/` to `arch/x86_64/`
- Add PE/COFF conversion step for EFI application

## Phase 2: EFI Entry Point Implementation

### 2.1 Create EFI Entry Point
- Replace `arch/i386/boot.s` with EFI entry point
- Implement `efi_main()` function with correct signature:
  ```c
  efi_status_t efi_main(efi_handle_t image_handle, efi_system_table_t* system_table)
  ```
- Remove Multiboot header from assembly
- Create proper PE/COFF header for UEFI compatibility

### 2.2 Update Kernel C Entry Point
- Modify `kernel_main()` to accept EFI parameters instead of Multiboot
- Update function signature to:
  ```c
  void kernel_main(efi_handle_t image_handle, efi_system_table_t* system_table)
  ```
- Handle EFI system table parsing for memory map, console output, etc.

### 2.3 Implement EFI Services Abstraction
- Create EFI wrapper functions for:
  - Memory information retrieval (replace multiboot memory map)
  - Console output (text/graphics) - replace current terminal drivers
  - File system access (if needed for future features)
  - Exit boot services when transitioning to kernel mode

## Phase 3: Memory Management Changes

### 3.1 Update Linker Script
- Modify `linker.ld` for 64-bit addressing
- Change kernel base address to higher half (0xFFFFFFFF80000000+)
- Add proper section alignments for x86_64 (4K pages)
- Update entry point for EFI application

### 3.2 Update Memory Management
- Adapt PMM (Physical Memory Manager) for 64-bit addresses
- Update VMM (Virtual Memory Manager) for 4-level paging
- Modify heap initialization for 64-bit pointers
- Combine with higher-half implementation (0xC0000000+ virtual addresses)

## Phase 4: System Initialization Updates

### 4.1 Replace GDT Setup
- Remove manual GDT setup (provided by EFI)
- Use EFI services to get initial system state
- Create new GDT only for kernel-specific segments

### 4.2 Update IDT Setup
- Keep interrupt handling initialization
- Adapt for x86_64 interrupt handling
- Update all assembly interrupt handlers for 64-bit

### 4.3 Replace Multiboot Information Parsing
- Use EFI services to get memory map
- Replace multiboot_info_t parsing with EFI system table usage
- Implement proper memory map processing for EFI exit boot services

## Phase 5: Build System and Deployment

### 5.1 Create EFI Application Target
- Add Makefile target to create `.efi` executable
- Implement PE32+ format generation (required by UEFI)
- Use `objcopy` to convert ELF to PE32+ format
- Sign EFI binary if required

### 5.2 Update QEMU Testing
- Add EFI-enabled QEMU run target
- Use OVMF (Open Virtual Machine Firmware) for UEFI support
- Update run/debug commands to use EFI boot
- Add EFI-specific debugging support

### 5.3 Create EFI Bootable Image
- Generate FAT32 filesystem image with EFI application
- Create directory structure: `/EFI/BOOT/BOOTX64.EFI`
- Update ISO creation process for EFI boot
- Support both legacy BIOS and EFI boot modes

## Phase 6: Code Modernization

### 6.1 Update Data Types
- Change `kuint32_t` to `kuint64_t` where appropriate
- Update pointer arithmetic for 64-bit addresses
- Review all struct alignments and packing for x86_64

### 6.2 Update Assembly Code
- Convert all assembly files to 64-bit equivalents
- Update register names and calling conventions
- Adapt to x86_64 instruction set
- Change from `arch/i386/` to `arch/x86_64/`

### 6.3 Update C Code
- Update all pointer operations for 64-bit
- Modify data structures for 64-bit compatibility
- Review and update all bit manipulation code

## Phase 7: Integration with Higher-Half Kernel

### 7.1 Combine with Higher-Half Implementation
- Use EFI's initial page tables as starting point
- Implement transition from EFI-provided environment to kernel-controlled higher-half
- Ensure compatibility with your existing higher-half TODO items

### 7.2 Memory Layout Coordination
- Align EFI memory map with your higher-half memory layout
- Ensure proper virtual-to-physical address translation
- Coordinate HEAP_VIRTUAL_START (0xD0000000) with EFI memory layout

## Expected Benefits

1. **Simplified Boot Process**: EFI provides many services that you currently implement manually
2. **Better Hardware Support**: EFI has standardized interfaces for various hardware
3. **Modern Architecture**: x86_64 provides more registers and better performance
4. **Industry Standard**: EFI is the modern boot standard for x86 systems
5. **Eliminates Multiboot Dependency**: No longer need GRUB for booting
6. **Better Debugging**: EFI applications can be easier to debug in some contexts

## Potential Challenges

1. **Toolchain Setup**: May require building or obtaining x86_64-elf toolchain
2. **Learning Curve**: Need to understand EFI specifications and APIs
3. **Debugging Complexity**: Different debugging setup for EFI applications
4. **Compatibility Testing**: Need to test on both EFI and legacy BIOS systems
5. **Memory Management Complexity**: Coordinating EFI memory map with kernel memory management
6. **Assembly Code Rewrite**: All assembly code needs to be ported to x86_64

## Implementation Order Recommendation

1. Set up x86_64 toolchain and verify basic compilation
2. Create minimal EFI entry point with basic console output
3. Implement EFI memory map parsing and exit boot services
4. Set up basic 64-bit higher-half kernel with EFI entry
5. Port memory management (PMM/VMM) to 64-bit
6. Port all drivers and subsystems to 64-bit
7. Create bootable EFI image with proper FAT32 filesystem
8. Test and debug on both QEMU and real hardware if possible
9. Add advanced EFI features (filesystem access, etc.)

## Required Reading/Resources

- UEFI Specification (latest version)
- Intel x86_64 Architecture Manual
- GNU EFI (gnu-efi) source code as reference
- OSDev Wiki EFI/UEFI articles
- System V ABI for x86_64
- Intel Software Developer Manuals

## Relationship to Current Development Goals

This transition aligns well with your existing TODO items:
- Higher-half kernel support (already planned)
- Virtual memory management improvements
- Process management foundation (64-bit required for modern process models)
- User mode transition (easier with EFI services)
- Advanced memory management features (simplified with EFI)

The EFI transition will actually simplify many of the items in your TODO list by providing standardized services that you would otherwise need to implement yourself.