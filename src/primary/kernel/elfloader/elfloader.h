#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <modules/modules.h>

typedef struct ELF_Ident {
    // Must be 0x7F, 'E', 'L', 'F'
    u8  magic_number[4];
    // 1 = 32bit, 2 = 64bit
    u8  arch;
    // 1 = LE, 2 = BE
    u8  endian;
    u8  header_version;
    // 0 for SysV (which I assume i686-astatine autodefaults to anyway)
    u8  ABI;
    u8  reserved[8];
} PACKED ELF_Ident;

// defines the first like 52 bytes of an ELF
// file header.
// will be replaced when I come out with my own
// executable format (that is NOT mach-o)
typedef struct ELF_Header {
    ELF_Ident ident;
    // 1 = relocatable, 2 = executable, 3 = shared, 4 = core dump
    u16 type;
    // instruction set: we want 0x03 for x86
    u16 instruction;
    u32 elf_version;
    // Entry point address
    u32 entry_offset;
    // IMPORTANT: where the program header table starts in the file
    u32 program_header_offset;
    // IMPORTANT: where the section header table starts in the file
    u32 section_header_offset;
    // useless for x86
    u32 flags;
    // size of the header we are in
    u16 header_size;
    // size*count gets us the length of the program header
    u16 program_entry_size;
    u16 program_entry_count;
    // same with section entry
    u16 section_entry_size;
    u16 section_entry_count;
    // and I have no idea what this even does.
    u16 section_string_table_section_index;
} PACKED ELF_Header;

typedef enum ELF_Program_Header_Type {
    ELF_PT_NULL = 0,
    ELF_PT_LOAD = 1,
    ELF_PT_DYNAMIC = 2,
    ELF_PT_INTERP = 3,
    ELF_PT_NOTE = 4,
    ELF_PT_SHLIB = 5,
    ELF_PT_PHDR = 6
} ELF_Program_Header_Type;

typedef struct ELF_Program_Header {
    // This is ultra important because this tells us
    // how to actually load the program and also the vmm.
    // 0 = null,
    // 1 = load statically
    // 2 = load dynamically
    // 3 = interpreter info
    // 4 = notes, 5 = unused, 6 = program header table
    u32 type;
    // At which section of the file that this segment starts
    u32 offset_in_file;
    // The virtual address to load this segment at
    u32 virtual_addr;
    // Sometimes it might want us to load to a physical address?
    // idfk apparently it doesnt matter
    u32 physical_addr;
    // Size in file (size of data)
    u32 size_file;
    // Size in memory (probably including extras)
    u32 size_mem;
    // 1=read, 2=write, 4=execute
    u32 permissions;
    // alignment requirements
    u32 alignment;
} PACKED ELF_Program_Header;

int is_elf(char* file_path);
int elf_load_and_run(char* file_path);

#endif