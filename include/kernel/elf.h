#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include <libc/stdint.h>

// ELF Magic Number
#define ELF_MAGIC 0x464C457F // 0x7F 'E' 'L' 'F'

// ELF Identification (e_ident) indices
#define EI_MAG0     0       // Magic number byte 0
#define EI_MAG1     1       // Magic number byte 1
#define EI_MAG2     2       // Magic number byte 2
#define EI_MAG3     3       // Magic number byte 3
#define EI_CLASS    4       // File class (32-bit or 64-bit)
#define EI_DATA     5       // Data encoding (endianness)
#define EI_VERSION  6       // ELF header version
#define EI_OSABI    7       // OS/ABI identification
#define EI_ABIVERSION 8     // ABI version
#define EI_NIDENT   16      // Size of e_ident[]

// ELF File Classes
#define ELFCLASSNONE 0      // Invalid class
#define ELFCLASS32  1       // 32-bit objects
#define ELFCLASS64  2       // 64-bit objects

// ELF Data Encoding
#define ELFDATANONE 0       // Invalid data encoding
#define ELFDATA2LSB 1       // Little-endian
#define ELFDATA2MSB 2       // Big-endian

// ELF Object File Types
#define ET_NONE     0       // No file type
#define ET_REL      1       // Relocatable file
#define ET_EXEC     2       // Executable file
#define ET_DYN      3       // Shared object file
#define ET_CORE     4       // Core file

// ELF Machine Architectures
#define EM_NONE     0       // No machine
#define EM_386      3       // Intel 80386
#define EM_X86_64   62      // AMD x86-64

// Program Header Types
#define PT_NULL     0       // Unused entry
#define PT_LOAD     1       // Loadable segment
#define PT_DYNAMIC  2       // Dynamic linking information
#define PT_INTERP   3       // Path to interpreter
#define PT_NOTE     4       // Auxiliary information
#define PT_SHLIB    5       // Reserved
#define PT_PHDR     6       // Program header table itself
#define PT_TLS      7       // Thread-local storage segment

// Program Header Flags
#define PF_X        0x1     // Executable
#define PF_W        0x2     // Writable
#define PF_R        0x4     // Readable

// ELF Header Structure (32-bit)
typedef struct {
    kuint8_t  e_ident[EI_NIDENT]; // ELF identification
    kuint16_t e_type;             // Object file type
    kuint16_t e_machine;          // Machine type
    kuint32_t e_version;          // Object file version
    kuint32_t e_entry;            // Entry point address
    kuint32_t e_phoff;            // Program header table file offset
    kuint32_t e_shoff;            // Section header table file offset
    kuint32_t e_flags;            // Processor-specific flags
    kuint16_t e_ehsize;           // ELF header size in bytes
    kuint16_t e_phentsize;        // Program header table entry size
    kuint16_t e_phnum;            // Program header table entry count
    kuint16_t e_shentsize;        // Section header table entry size
    kuint16_t e_shnum;            // Section header table entry count
    kuint16_t e_shstrndx;         // Section header string table index
} __attribute__((packed)) elf_header_t;

// Program Header Table Entry Structure (32-bit)
typedef struct {
    kuint32_t p_type;           // Type of segment
    kuint32_t p_offset;         // Segment file offset
    kuint32_t p_vaddr;          // Segment virtual address
    kuint32_t p_paddr;          // Segment physical address (ignored for user programs)
    kuint32_t p_filesz;         // Segment size in file
    kuint32_t p_memsz;          // Segment size in memory
    kuint32_t p_flags;          // Segment flags
    kuint32_t p_align;          // Segment alignment
} __attribute__((packed)) elf_program_header_t;

#endif // KERNEL_ELF_H
