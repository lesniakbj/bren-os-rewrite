#include <libc/sysstd.h>

int main() {
    vfs_write(1, "Hello from user mode!", 21);
    proc_exit(0);
}