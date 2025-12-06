#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <modules/modules.h>
#include <modules/dynarray.h>
#include <fat32/fat32.h>

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

typedef enum {
    DT_NULL            = 0,     /* Marks end of dynamic array */
    DT_NEEDED          = 1,     /* Name of needed library */
    DT_PLTRELSZ        = 2,     /* Size of PLT relocation table */
    DT_PLTGOT          = 3,     /* Processor-dependent */
    DT_HASH            = 4,     /* Address of symbol hash table */
    DT_STRTAB          = 5,     /* Address of string table */
    DT_SYMTAB          = 6,     /* Address of symbol table */
    DT_RELA            = 7,     /* Address of Rela reloc table */
    DT_RELASZ          = 8,     /* Size of Rela table */
    DT_RELAENT         = 9,     /* Size of each Rela entry */
    DT_STRSZ           = 10,    /* Size of string table */
    DT_SYMENT          = 11,    /* Size of symbol table entry */
    DT_INIT            = 12,    /* Address of initialization func */
    DT_FINI            = 13,    /* Address of termination func */
    DT_SONAME          = 14,    /* Shared object name */
    DT_RPATH           = 15,    /* Library search path (deprecated) */
    DT_SYMBOLIC        = 16,    /* Symbol resolution changes */
    DT_REL             = 17,    /* Address of Rel reloc table */
    DT_RELSZ           = 18,    /* Size of Rel table */
    DT_RELENT          = 19,    /* Size of each Rel entry */
    DT_PLTREL          = 20,    /* Type of PLT relocation */
    DT_DEBUG           = 21,    /* Debugging entry (unused) */
    DT_TEXTREL         = 22,    /* Relocations to text segment */
    DT_JMPREL          = 23,    /* Address of PLT reloc table */
    DT_BIND_NOW        = 24,    /* Binder handles immediately */
    DT_INIT_ARRAY      = 25,    /* Array of init function pointers */
    DT_FINI_ARRAY      = 26,    /* Array of fini function pointers */
    DT_INIT_ARRAYSZ    = 27,    /* Size of DT_INIT_ARRAY */
    DT_FINI_ARRAYSZ    = 28,    /* Size of DT_FINI_ARRAY */
    DT_RUNPATH         = 29,    /* Search path for libraries */
    DT_FLAGS           = 30,    /* Flags for the object */
    DT_ENCODING        = 31,    /* Start of encoded range */

    /* OS / processor-specific */
    DT_PREINIT_ARRAY   = 32,    /* Pre-initialization array */
    DT_PREINIT_ARRAYSZ = 33,    /* Size of preinit array */

    /* GNU extensions */
    DT_FLAGS_1         = 0x6ffffffb,
    DT_VERDEF          = 0x6ffffffc,
    DT_VERDEFNUM       = 0x6ffffffd,
    DT_VERNEED         = 0x6ffffffe,
    DT_VERNEEDNUM      = 0x6fffffff,

    /* Sun extensions */
    DT_AUXILIARY       = 0x7ffffffd,
    DT_FILTER          = 0x7fffffff
} Elf32_Dynamic_Tag;

typedef struct ELF_Dynamic_Entry {
    i32 tag;
    u32 val;
} PACKED ELF_Dynamic_Entry;

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

enum ShT_Types {
    SHT_NULL    = 0,   // Null section
    SHT_PROGBITS= 1,   // Program information
    SHT_SYMTAB  = 2,   // Symbol table
    SHT_STRTAB  = 3,   // String table
    SHT_RELA    = 4,   // Relocation (w/ addend)
    SHT_NOBITS  = 8,   // Not present in file, just zeroes
    SHT_REL     = 9,   // Relocation (no addend)
};

enum ShT_Flags {
    SHF_WRITE        = 0x1,
    SHF_ALLOC        = 0x2,
    SHF_EXECINSTR    = 0x4,
};

typedef struct ELF_Section_Header {
    // String for header (we use this to compare)
    // Another point of interest is that the sh_name field does 
    // not point directly to a string, instead it gives the offset 
    // of a string in the section name string table
    u32 name;
    // ShT_Types
    u32 type;
    // 0x01: writable, 0x02: alloc, 0x04: executable
    u32 flags;
    // actual location of what we need
    u32 addr;
    // file offset
    u32 offset;
    // size of section
    u32 size;
    // link to another section
    u32 link;
    // extra info
    u32 info;
    // alignment
    u32 addr_align;
    // size of entries if section has a table
    u32 entry_size;
} PACKED ELF_Section_Header;

typedef struct ELF_Relocation_Entry {
    u32 offset;
    u32 info;
} PACKED ELF_Relocation_Entry;

typedef struct ELF_Relocation_Entry_Addend {
    u32 offset;
    u32 info;
    i32 addend;
} PACKED ELF_Relocation_Entry_Addend;

enum ELF_Relocation_Types_i386 {
    R_386_NONE        = 0,
    R_386_32          = 1,
    R_386_PC32        = 2,
    R_386_GOT32       = 3,
    R_386_PLT32       = 4,
    R_386_COPY        = 5,
    R_386_GLOB_DAT    = 6,
    R_386_JMP_SLOT    = 7,
    R_386_RELATIVE    = 8,
    R_386_GOTOFF      = 9,
    R_386_GOTPC       = 10,
    R_386_32PLT      = 11,
    R_386_TLS_TPOFF  = 14,
    R_386_TLS_IE     = 15,
    R_386_TLS_GOTIE  = 16,
};

int is_elf(char* file_path);
bool load_program_headers_elf(File* file, Dynarray* addrs);
bool load_section_headers_elf(File* file, Dynarray* addrs);
int elf_load_and_run(char* file_path);

#endif