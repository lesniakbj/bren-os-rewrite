#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

#include <kernel/multiboot.h>
#include <kernel/log.h>
#include <arch/i386/time.h>
#include <arch/i386/gdt.h>
#include <arch/i386/idt.h>
#include <arch/i386/pmm.h>
#include <arch/i386/pic.h>

#ifdef DEBUG
void debug_time(cmos_time_t current_time);
void debug_gdt();
void debug_pic();
void debug_idt();
void debug_multiboot_header(multiboot_info_t *mbi);
void debug_pmm(pmm_init_status_t *pmm_status);
void debug_proc_test();
void test_heap_allocations();
#endif

#endif