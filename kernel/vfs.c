#include <kernel/vfs.h>
#include <drivers/terminal.h>
#include <drivers/serial.h>

static file_node_t dev_terminal_node;
static file_node_t dev_com_io1_node;
static file_node_t dev_com_io2_node;

void vfs_init() {
    // Initialize an output to a terminal
    dev_terminal_node.path = "/dev/term";
    dev_terminal_node.write = terminal_write;
    dev_terminal_node.write_string = terminal_write_string;
    dev_terminal_node.read = NULL;

    // Initialize output to a COM port
    SERIAL_VFS_ADAPTER(scom1, SERIAL_COM1);
    dev_com_io1_node.path = "/dev/serial/com1";
    dev_com_io1_node.write = scom1_write;
    dev_com_io1_node.write_string = scom1_write_string;
    dev_com_io1_node.read = NULL;

    // Initialize output to a COM port
    SERIAL_VFS_ADAPTER(scom2, SERIAL_COM2);
    dev_com_io2_node.path = "/dev/serial/com2";
    dev_com_io2_node.write = scom2_write;
    dev_com_io2_node.write_string = scom2_write_string;
    dev_com_io2_node.read = NULL;
}

file_node_t* vfs_get_terminal_node() {
    return &dev_terminal_node;
}

file_node_t* vfs_get_serial_com1_node() {
    return &dev_com_io1_node;
}

file_node_t* vfs_get_serial_com2_node() {
    return &dev_com_io2_node;
}