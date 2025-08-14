#include <kernel/vfs.h>
#include <drivers/terminal.h> // test as a first driver file

static file_node_t dev_terminal_node;

void vfs_init() {
    // Initialize an output to a terminal
    dev_terminal_node.write = terminal_write;
    dev_terminal_node.read = NULL;
}

file_node_t* vfs_get_terminal_node() {
    return &dev_terminal_node;
}