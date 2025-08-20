#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <libc/stdint.h>

typedef struct file {
    const char* path;
    kint32_t (*write)(const char* buf, size_t count);
    kint32_t (*write_string)(const char* buf);
    kint32_t (*read)(char* buf, size_t count);
    // int (*open)(const char* buf, size_t count);
    // int (*close)(const char* buf, size_t count);
    // int (*seek)(const char* buf, size_t count);
} file_node_t;

void vfs_init();
file_node_t* vfs_get_terminal_node();
file_node_t* vfs_get_serial_com1_node();
file_node_t* vfs_get_serial_com2_node();

#endif