#include <kernel/log.h>
#include <drivers/serial.h>
#include <drivers/screen.h>
#include <drivers/terminal.h>

static const char* levelNames[] = {
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO] =  "INFO",
    [LOG_LEVEL_WARN] =  "WARN",
    [LOG_LEVEL_ERR] =   "ERROR",
    [LOG_LEVEL_PANIC] = "PANIC",
};

#define SERIAL_LOG 1
#define SCREEN_LOG 1

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
    vformat_string(buffer, buff_size, format, args); // Call the core logic
    va_end(args); // Clean up the va_list we created
}

static void format_string(char* buffer, size_t buff_size, const char* format, va_list args) {
     // Simply delegate to the core logic
     vformat_string(buffer, buff_size, format, args);
     // Do NOT call va_end here
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
    va_list args;
    va_start(args, format);

    char call_info[128];
    format_string_simple(call_info, 128, "%s.%d:0x%x :: ", file, line, caller_addr);
#if SERIAL_LOG
    serial_write_string(SERIAL_COM1, call_info);
#endif
#if SCREEN_LOG
    terminal_write_string(call_info);
#endif

    char header[64];
    format_string_simple(header, 64, "[%s] :: ", levelNames[level]);
#if SERIAL_LOG
    serial_write_string(SERIAL_COM1, header);
#endif
#if SCREEN_LOG
    terminal_write_string(header);
#endif

    char buf[256];
    format_string(buf, 256, format, args);
#if SERIAL_LOG
    serial_write_string(SERIAL_COM1, buf);
#endif
#if SCREEN_LOG
    terminal_write_string(buf);
#endif
    va_end(args);
}