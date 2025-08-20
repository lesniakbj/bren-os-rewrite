#include <drivers/terminal.h>
#include <kernel/multiboot.h>
#include <kernel/sync.h>
#include <kernel/log.h>

// Functions referencing the text-mode terminal
extern void text_mode_console_init(void);
extern void text_mode_clear(void);
extern void text_mode_putchar(char c);
extern kint32_t text_mode_write(const char* data, size_t size);
extern void text_mode_writestring(const char* data);
extern void text_mode_setcolor(vga_color_t fg, vga_color_t bg);
extern void text_mode_scroll(kint32_t lines);

// Functions referencing the framebuffer (GUI) terminal
extern void framebuffer_console_init(multiboot_info_t* mbi);
extern void framebuffer_clear(void);
extern void framebuffer_putchar(char c);
extern kint32_t framebuffer_write(const char* data, size_t size);
extern void framebuffer_writestring(const char* data);
extern void framebuffer_setcolor(vga_color_t fg, vga_color_t bg);
extern void framebuffer_scroll(kint32_t lines);

// The currently active terminal driver
static struct terminal_driver active_driver;


// On init, check if we are using the framebuffer or not (GRUB will set this up for us)
void terminal_init(multiboot_info_t* mbi) {
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 12)) {
        if(mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
            framebuffer_console_init(mbi);
            active_driver.clear = framebuffer_clear;
            active_driver.putchar = framebuffer_putchar;
            active_driver.write = framebuffer_write;
            active_driver.writestring = framebuffer_writestring;
            active_driver.setcolor = framebuffer_setcolor;
            active_driver.scroll = framebuffer_scroll;
            return;
        }
    }

    text_mode_console_init();
    active_driver.clear = text_mode_clear;
    active_driver.putchar = text_mode_putchar;
    active_driver.write = text_mode_write;
    active_driver.writestring = text_mode_writestring;
    active_driver.setcolor = text_mode_setcolor;
    active_driver.scroll = text_mode_scroll;
}

void terminal_clear(void) {
    active_driver.clear();
}

void terminal_putchar(char c) {
    active_driver.putchar(c);
}

kint32_t terminal_write(const char* data, size_t size) {
    return active_driver.write(data, size);
}

static spinlock_t terminal_write_lock = SPINLOCK_UNLOCKED;

void terminal_write_string(const char* data) {
    spinlock_acquire(&terminal_write_lock);
    active_driver.writestring(data);
    spinlock_release(&terminal_write_lock);
}

void terminal_setcolor(vga_color_t fg, vga_color_t bg) {
    active_driver.setcolor(fg, bg);
}

void terminal_scroll(kint32_t lines) {
    active_driver.scroll(lines);
}