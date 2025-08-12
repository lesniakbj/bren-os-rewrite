# OS Development TODO List

## Virtual Memory Management
- [x] Implement vmm_map_page() function
- [x] Implement vmm_unmap_page() function
- [x] Implement vmm_get_physical_addr() function
- [ ] Add support for different page sizes (4MB pages)
- [ ] Implement copy-on-write (COW) mechanism
- [ ] Add memory protection features
- [ ] Implement higher-half kernel support

## Heap Memory Management
- [ ] Create heap.c implementation file
- [ ] Implement heap_init() function
- [ ] Implement kmalloc() function
- [ ] Implement kfree() function
- [ ] Implement krealloc() function
- [ ] Design data structures for tracking allocated blocks
- [ ] Implement different allocation strategies (first-fit, best-fit)
- [ ] Add memory leak detection (optional for debugging)

## Process Management Foundation
- [ ] Define process control block (PCB) structure
- [ ] Implement basic process creation/destruction
- [ ] Create process table for tracking processes
- [ ] Implement basic scheduling algorithm (round-robin)

## User Mode Transition
- [ ] Set up separate address spaces for kernel/user
- [ ] Implement system call interface
- [ ] Create mechanism for switching to user mode
- [ ] Set up user stack and memory layout

## System Call Interface
- [ ] Implement basic system calls (write, read, exit)
- [ ] Create syscall dispatch mechanism
- [ ] Add parameter validation for security
- [ ] Implement syscall table

## File System
- [ ] Design simple file system structure
- [ ] Implement basic file operations (create, read, write, delete)
- [ ] Add directory support
- [ ] Create file descriptor table

## Advanced Memory Management
- [ ] Implement demand paging
- [ ] Add page replacement algorithms (LRU)
- [ ] Implement memory-mapped files
- [ ] Add swap space support

## Enhanced Device Drivers
- [ ] Implement disk driver (ATA/IDE)
- [ ] Add serial port driver for debugging
- [ ] Implement timer driver for scheduling
- [ ] Add network driver support

## Inter-Process Communication
- [ ] Implement basic IPC mechanisms (pipes, messages)
- [ ] Add shared memory support
- [ ] Create synchronization primitives (mutexes, semaphores)

## Advanced Features
- [ ] Implement virtual file system (VFS)
- [ ] Add loadable kernel modules
- [ ] Implement basic networking stack
- [ ] Add multitasking with process priorities