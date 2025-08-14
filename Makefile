# Cross-compiler and tools
CC = .toolchain/cross/bin/i686-elf-gcc
AS = .toolchain/cross/bin/i686-elf-as

# Compiler and linker flags
CFLAGS = -Iinclude -std=gnu99 -ffreestanding -nostdinc -O2 -Wall -Wextra -g -MMD -MP -msoft-float -DDEBUG
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

# Source files
C_SOURCES = \
	kernel/kernel.c \
	kernel/acpi.c \
	kernel/heap.c \
	kernel/proc.c \
	kernel/syscall.c \
	kernel/vfs.c \
	kernel/time.c \
	kernel/log.c \
	kernel/debug.c \
	libk/udivdi3.c \
	drivers/terminal.c \
	drivers/pci.c \
	drivers/hpet.c \
	drivers/pit.c \
	drivers/screen.c \
	drivers/serial.c \
	drivers/keyboard.c \
	drivers/mouse.c \
	libc/stdlib.c \
	libc/strings.c \
	libc/sysstd.c \
	arch/i386/time.c \
	arch/i386/memory.c \
	arch/i386/pmm.c \
	arch/i386/vmm.c \
	arch/i386/idt.c \
	arch/i386/gdt.c \
	arch/i386/pic.c \
	arch/i386/interrupts.c \
	drivers/text_mode_console.c \
	drivers/framebuffer_console.c

ASM_SOURCES = \
	arch/i386/boot.s \
	arch/i386/gdt_asm.s \
	arch/i386/idt_asm.s \
	arch/i386/vmm_asm.s \
	arch/i386/isr_handlers.s \
	arch/i386/io_asm.s \
	arch/i386/interrupts_asm.s

# Object files
OBJ_DIR = .toolchain/build/obj
C_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
ASM_OBJS = $(patsubst %.s,$(OBJ_DIR)/%.o,$(ASM_SOURCES))
OBJ_FILES = $(C_OBJS) $(ASM_OBJS)

$(info OBJ_FILES: $(OBJ_FILES))

# Dependency files
DEPS = $(OBJ_FILES:.o=.d)

# Output files and directories
TARGET_BIN = test-os.bin
ISO_FILE = test-os.iso
BUILD_DIR = .toolchain/build
ISO_DIR = .toolchain/iso

all: $(ISO_FILE)

# Create the final ISO image
$(ISO_FILE): $(BUILD_DIR)/$(TARGET_BIN) grub/grub.cfg
	@echo "Creating ISO image..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/$(TARGET_BIN) $(ISO_DIR)/boot/
	@cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)
	@echo "Build complete! ISO created at $(ISO_FILE)"

# Link the kernel
$(BUILD_DIR)/$(TARGET_BIN): $(OBJ_FILES)
	@echo "Linking kernel..."
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$(TARGET_BIN) $(OBJ_FILES)

# Compile C files
$(OBJ_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble assembly files
$(OBJ_DIR)/%.o: %.s
	@echo "Assembling $<..."
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) $< -o $@

# Include dependency files
-include $(DEPS)

# Clean up build files
clean:
	@echo "Cleaning up..."
	@rm -f $(ISO_FILE)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)

run: $(ISO_FILE)
	@echo "Running OS in QEMU..."
	@qemu-system-i386 -cdrom $(ISO_FILE) -serial stdio -machine q35,hpet=on

debug: $(ISO_FILE)
	@echo "Running OS in QEMU with GDB support..."
	@qemu-system-i386 -cdrom $(ISO_FILE) -s -S -serial stdio -machine q35,hpet=on

monitor: $(ISO_FILE)
	@echo "Running OS in QEMU with Monitoring enabled..."
	@qemu-system-i386 -cdrom $(ISO_FILE) -monitor stdio -serial stdio -machine q35,hpet=on #-device piix3-usb-uhci,id=usb -device usb-kbd,id=keyboard -machine hpet=on

.PHONY: all clean run debug
