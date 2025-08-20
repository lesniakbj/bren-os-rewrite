#include <kernel/log.h>
#include <kernel/sync.h>
#include <drivers/serial.h>
#include <drivers/screen.h>
#include <drivers/terminal.h>

static spinlock_t log_lock = SPINLOCK_UNLOCKED;

static const char* levelNames[] = {
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO] =  "INFO",
    [LOG_LEVEL_WARN] =  "WARN",
    [LOG_LEVEL_ERR] =   "ERROR",
    [LOG_LEVEL_PANIC] = "PANIC",
};

#define SERIAL_LOG 1
#define SCREEN_LOG 1

static inline bool check_interrupts_enabled() {
    kuint64_t flags;
    asm volatile (
        "pushf \n"        // Push the EFLAGS register onto the stack
        "pop %0 \n"       // Pop the value from the stack into the 'flags' variable
        : "=r" (flags)  // Output operand: 'flags' is a general-purpose register
    );
    return flags & (1 << 9);
}

// Similar to a simplified sprintf() but without flags/width/precision/etc...
// Implementation based on various parts of: https://stackoverflow.com/questions/16647278/minimal-implementation-of-sprintf-or-printf
static void vformat_string(char* buffer, size_t buff_size, const char* format, va_list args) {
    kuint64_t i = 0;
    while (*format != '\0' && i < buff_size - 1) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char* s = va_arg(args, char*);
                while (*s != '\0' && i < buff_size - 1) {
                    buffer[i++] = *s++;
                }
            } else if (*format == 'd') {
                kint32_t d = va_arg(args, kint32_t);
                char num_buf[32];
                itoa(num_buf, 'd', d);
                char* s = num_buf;
                while (*s != '\0' && i < buff_size - 1) {
                    buffer[i++] = *s++;
                }
            } else if (*format == 'x') {
                kuint32_t x = va_arg(args, kuint32_t);
                char num_buf[32];
                itoa(num_buf, 'x', x);
                char* s = num_buf;
                while (*s != '\0' && i < buff_size - 1) {
                    buffer[i++] = *s++;
                }
            } else if (*format == '%') {
                buffer[i++] = '%';
            }
        } else {
            buffer[i++] = *format;
        }
        format++;
    }
    buffer[i] = '\0';
}

static void format_string_simple(char* buffer, size_t buff_size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vformat_string(buffer, buff_size, format, args);
    va_end(args);
}

static void format_string(char* buffer, size_t buff_size, const char* format, va_list args) {
     // Simply delegate to the core logic
     vformat_string(buffer, buff_size, format, args);
}

void log_init(multiboot_info_t *mbi) {
#if SERIAL_LOG
    serial_init(SERIAL_COM1);
#endif
#if SCREEN_LOG
    screen_init(mbi);
#endif

    LOG_INFO("Booting %s-kernel %s (Built %s %s)\n", "TestOS", "0.0.1v", __DATE__, __TIME__);
    LOG_INFO("Copyright (C) 2025 Brendan Lesniak. MIT Licensed.\n");
}

void log_print(log_level_t level, const char* file, void* caller_addr, int line, const char* format, ...) {
    static bool return_lock = false;
    if(check_interrupts_enabled()) {
        spinlock_acquire(&log_lock);
        return_lock = true;
    }

    va_list args;
    va_start(args, format);

    // For each log line, we concantenate 3 pieces of information...
    // ... Call info (File, Line, Caller Address) ...
    char call_info[128];
    format_string_simple(call_info, 128, "%s.%d:0x%x :: ", file, line, caller_addr);
#if SERIAL_LOG
    serial_write_string(SERIAL_COM1, call_info);
#endif
#if SCREEN_LOG
    terminal_write_string(call_info);
#endif

    // ... Log Level Header ...
    char header[64];
    format_string_simple(header, 64, "[%s] :: ", levelNames[level]);
#if SERIAL_LOG
    serial_write_string(SERIAL_COM1, header);
#endif
#if SCREEN_LOG
    terminal_write_string(header);
#endif

    // ... The Log Message
    char buf[256];
    format_string(buf, 256, format, args);
#if SERIAL_LOG
    serial_write_string(SERIAL_COM1, buf);
#endif
#if SCREEN_LOG
    terminal_write_string(buf);
#endif

    va_end(args);

    if(return_lock)
        spinlock_release(&log_lock);
}