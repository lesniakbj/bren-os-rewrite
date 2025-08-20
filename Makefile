# Include default build variables
-include Make.defaults

# --- Kernel Sources ---
# Source directories
KRNL_SRC_DIRS = kernel libk drivers libc arch/i386

# Find all C and assembly source files
KRNL_C_FILES = $(foreach dir,$(KRNL_SRC_DIRS),$(wildcard $(dir)/*.c))
KRNL_ASM_FILES = $(foreach dir,$(KRNL_SRC_DIRS),$(wildcard $(dir)/*.s))

# Object files
KRNL_C_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(KRNL_C_FILES))
KRNL_ASM_OBJS = $(patsubst %.s,$(OBJ_DIR)/%.o,$(KRNL_ASM_FILES))
KRNL_OBJS = $(KRNL_C_OBJS) $(KRNL_ASM_OBJS)

# --- User Sources ---
USER_SRC_DIRS = usr

USER_C_FILES = $(foreach dir,$(USER_SRC_DIRS),$(wildcard $(dir)/*.c))
USER_ASM_FILES = $(foreach dir,$(USER_SRC_DIRS),$(wildcard $(dir)/*.s))

USER_C_OBJS = $(patsubst %.c,$(USER_OBJ_DIR)/%.o,$(USER_C_FILES))
USER_ASM_OBJS = $(patsubst %.s,$(USER_OBJ_DIR)/%.o,$(USER_ASM_FILES))
USER_OBJS = $(USER_C_OBJS) $(USER_ASM_OBJS)

USER_BINS = $(patsubst %.c,$(USER_BIN_DIR)/%,$(notdir $(USER_C_FILES)))

# User library files
USER_LIB_C_FILES = libc/sysstd.c
USER_LIB_OBJS = $(patsubst %.c,$(USER_OBJ_DIR)/%.o,$(USER_LIB_C_FILES))

# --- Build Targets ---
# Dependency files
DEPS = $(KRNL_OBJS:.o=.d) $(USER_OBJS:.o=.d) $(USER_LIB_OBJS:.o=.d)

# Output files
TARGET_BIN = $(BIN_DIR)/$(PROJECT_NAME).bin
ISO_FILE = $(BUILD_DIR)/$(PROJECT_NAME).iso

.PHONY: all clean user_programs run debug gdb_debug monitor

all: $(ISO_FILE)

user_programs: $(USER_BINS)

# Create the final ISO image
$(ISO_FILE): $(TARGET_BIN) grub/grub.cfg.in user_programs
	@echo "Creating ISO image..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(TARGET_BIN) $(ISO_DIR)/boot/
	@sed 's,@@KERNEL_BIN@@,$(notdir $(TARGET_BIN)),g' grub/grub.cfg.in > $(ISO_DIR)/boot/grub/grub.cfg
	@mkdir -p $(ISO_DIR)/usr/bin
	@cp $(USER_BINS) $(ISO_DIR)/usr/bin/
	@grub-mkrescue -o $@ $(ISO_DIR)
	@echo "Build complete! ISO created at $@"

# --- Kernel Build Rules ---
# Link the kernel
$(TARGET_BIN): $(KRNL_OBJS)
	@echo "Linking kernel..."
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(KRNL_OBJS)

# Compile kernel C files
$(OBJ_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble kernel assembly files
$(OBJ_DIR)/%.o: %.s
	@echo "Assembling $<..."
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) $< -o $@

# --- User Program Build Rules ---
# Link user programs
$(USER_BIN_DIR)/%: $(USER_OBJ_DIR)/%.o $(USER_LIB_OBJS)
	@echo "Linking user program $<..."
	@mkdir -p $(@D)
	@$(CC) $(USER_LDFLAGS) -o $@ $^

# Compile user C files
$(USER_OBJ_DIR)/%.o: usr/%.c
	@echo "Compiling user program $<..."
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -c $< -o $@

# Compile user C library files
$(USER_OBJ_DIR)/libc/sysstd.o: libc/sysstd.c
	@echo "Compiling user library $<..."
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -c $< -o $@

# --- Housekeeping ---
# Include dependency files
-include $(DEPS)

# Clean up build files
clean:
	@echo "Cleaning up..."
	@rm -f $(ISO_FILE) $(TARGET_BIN)
	@rm -rf $(OBJ_DIR) $(ISO_DIR) $(USER_OBJ_DIR) $(USER_BIN_DIR)

# --- QEMU Targets ---
# Run QEMU
run:
	@echo "Running OS in QEMU..."
	@$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS)

# Run QEMU with GDB server
debug:
	@echo "Running OS in QEMU with GDB support..."
	@$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS) $(QEMU_DEBUG_FLAGS)

gdb_debug:
	@echo "Attaching GDB..."
	@gdb -x gdb.script

# Run QEMU with monitor
monitor:
	@echo "Running OS in QEMU with Monitoring enabled..."
	@$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS) -monitor stdio