#include "elfloader.h"
#include <fat32/fat32.h>
#include <display/simple/display.h>
#include <memory/paging.h>
#include <timer/PIT.h>

// check if the file path is an ELF file
// and make sure it can execute on this system.
// 0 is a valid ELF, 1 = not elf, 2 = wrong arch
// 3 = failed to read file

int temp;

extern void entryuser32(u32 addr, u32 useresp);
int is_elf(char* file_path) {
    File file;
    if (fat_file_open(&file, file_path, FAT_READ) != 0) {
        printf("Failed to open file: %s\n", file_path);
        return 3;
    }

    ELF_Ident ident;
    if (fat_file_read(&file, &ident, sizeof(ELF_Ident), &temp) != 0) {
        fat_file_close(&file);
        printf("Failed to read ELF identification.\n");
        return 3;
    }

    // Check magic number
    if (ident.magic_number[0] != 0x7F || ident.magic_number[1] != 'E' ||
        ident.magic_number[2] != 'L' || ident.magic_number[3] != 'F') {
        printf("File is not a valid ELF file: %s, reads %x%c%c%c\n", file_path, ident.magic_number[0], ident.magic_number[1], ident.magic_number[2], ident.magic_number[3]);
        fat_file_close(&file);
        return 1;
    }

    // Check architecture (1 = 32bit)
    if (ident.arch != 1) {
        printf("ELF file is not for 32-bit architecture: %s\n", file_path);
        fat_file_close(&file);
        return 2;
    }

    fat_file_close(&file);
    return 0;
}

// Now we know it's an ELF file for this system,
// we can make a function that bootstraps it into
// memory and runs it.
int elf_load_and_run(char* file_path) {
    // First we open the file
    File file;
    if (fat_file_open(&file, file_path, FAT_READ) != 0) {
        printf("Failed to open ELF file: %s\n", file_path);
        return 1;
    }

    // Then we should read just the ELF header
    ELF_Header header;
    if (fat_file_read(&file, &header, sizeof(ELF_Header), &temp) != 0) {
        printf("Failed to read the ELF header.\n");
        fat_file_close(&file);  
        return 2;
    }

    // Now we should go through the list of 
    // program headers
    // seek to the program header offset
    ELF_Program_Header current_header;
    u32 addrs[header.program_entry_count];
    // now for each file, read it in
    for (u32 i = 0; i <= header.program_entry_count; i++) {
        fat_file_seek(&file, header.program_header_offset + i * sizeof(ELF_Program_Header), FAT_SEEK_START);
        if (fat_file_read(&file, &current_header, sizeof(ELF_Program_Header), &temp) != 0) {
            printf("Failed to read the ELF headers.\n");
            fat_file_close(&file);
            return 3;
        }
        // Check if we can load this segment
        if (current_header.type != ELF_PT_LOAD) continue;

        // we should allocate virtual memory for this segment
        // at the address they want us to.
        for (u32 addr = current_header.virtual_addr;
             addr < current_header.virtual_addr + current_header.size_mem;
             addr += 0x1000) {
            addrs[i] = addr;
            alloc_page(addr);
        }
        // Now we should read the segment data from the file
        fat_file_seek(&file, current_header.offset_in_file, FAT_SEEK_START);
        if (fat_file_read(&file, (void*)current_header.virtual_addr, current_header.size_file, &temp) != 0) {
            printf("Failed to read segment data.\n");
            fat_file_close(&file);
            return 3;
        }
    }
    fat_file_close(&file);

    // Before we run the program,
    // we need to initialise the program stack
    // which is two pages above the highest segment
    u32 highest_addr = 0;
    for (u32 i = 0; i < header.program_entry_count; i++) {
        if (addrs[i] > highest_addr) {
            highest_addr = addrs[i];
        }
    }
    if (highest_addr == 0) {
        printf("No loadable segments found in ELF file.\n");
        return 4;
    }
    // allocate two pages for stack
    alloc_page(highest_addr + 0x1000);
    alloc_page(highest_addr + 0x2000);

    printf("Loading usermode program in 2 seconds...");
    sleep(2000);
    clear_screen();
    printf("goodbye kernel! hello user!\n");

    // Now we loaded the file, let's run the program
    entryuser32(header.entry_offset, highest_addr + 0x2000);
    for (u32 i = 0; i < header.program_entry_count; i++) {
        free_page(addrs[i]);
    }
    return 0;
}
